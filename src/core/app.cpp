#include "app.h"
#include "file_ops.h"
#include "localization.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <thread>

namespace {
std::thread scan_worker;
std::atomic<bool> stop_scan{false};
}

namespace fs = std::filesystem;
using json = nlohmann::json;

static void set_status(const std::string& s, const std::string& detail = {}) {
    std::lock_guard<std::mutex> lock(g_data_mutex);
    status_msg = s;
    status_detail = detail;
}

bool set_config_path(char* target, const std::string& path) {
    try {
        auto normalized = normalize_user_path(path);
        if (normalized.size() >= PATH_BUF_SIZE) {
            set_status("path_too_long");
            return false;
        }
        std::lock_guard<std::mutex> lock(g_data_mutex);
        if (normalized != target) {
            if (target == src_path) { src_list.clear(); selected_src = -1; }
            if (target == dst_path) { dst_list.clear(); selected_dst = -1; }
        }
        std::memcpy(target, normalized.c_str(), normalized.size() + 1);
        return true;
    } catch (const ConfigFileError& e) {
        set_status(e.key, e.detail);
        return false;
    } catch (const std::exception& e) {
        set_status("error", e.what());
        return false;
    }
}

static fs::path settings_path()  { return fs::u8path(config_dir()) / SETTINGS_FILE; }
static fs::path nick_cache_path(){ return fs::u8path(config_dir()) / CACHE_FILE; }
static fs::path avatar_url_cache_path() { return fs::u8path(config_dir()) / AVATAR_URL_CACHE_FILE; }

void load_settings() {
    auto default_path = default_steam_userdata();
    set_config_path(src_path, default_path);
    set_config_path(dst_path, default_path);
#ifdef _WIN32
    const auto registry_settings = read_registry_settings();
    const bool migrate_settings = registry_settings.empty();
    const bool have_settings = !registry_settings.empty() || fs::exists(settings_path());
#else
    const bool have_settings = fs::exists(settings_path());
#endif
    if (have_settings) {
        try {
            json j;
#ifdef _WIN32
            if (!registry_settings.empty()) j = json::parse(registry_settings);
            else {
                std::ifstream f(settings_path());
                f >> j;
            }
#else
            std::ifstream f(settings_path());
            f >> j;
#endif
            std::string s = j.value("src", default_path);
            std::string d = j.value("dst", default_path);
            g_theme = std::clamp(j.value("theme", 0), 0, 4);
            set_language(j.value("language", "en"));
            set_config_path(src_path, s);
            set_config_path(dst_path, d);
#ifdef _WIN32
            if (migrate_settings && write_registry_settings(j.dump())) {
                std::error_code cleanup_error;
                fs::remove(settings_path(), cleanup_error);
            }
#endif
        } catch (...) {}
    }
    if (fs::exists(nick_cache_path())) {
        try {
            std::ifstream f(nick_cache_path());
            json j; f >> j;
            std::lock_guard<std::mutex> lock(g_data_mutex);
            for (auto& el : j.items()) nick_cache[el.key()] = el.value();
        } catch (...) {}
    }
    if (fs::exists(avatar_url_cache_path())) {
        try {
            std::ifstream f(avatar_url_cache_path());
            json j; f >> j;
            std::lock_guard<std::mutex> lock(g_data_mutex);
            for (auto& el : j.items()) avatar_url_cache[el.key()] = el.value();
        } catch (...) {}
    }
}

void save_settings() {
    static std::mutex save_mutex;
    std::lock_guard<std::mutex> save_lock(save_mutex);
    std::string source_path, destination_path;
    std::map<std::string, std::string> nick_copy;
    std::map<std::string, std::string> avatar_copy;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        source_path = src_path;
        destination_path = dst_path;
        nick_copy = nick_cache;
        avatar_copy = avatar_url_cache;
    }
    try {
        json settings = {{"src", source_path},{"dst", destination_path},{"theme", g_theme.load()},{"language", language_code()}};
#ifdef _WIN32
        if (write_registry_settings(settings.dump())) {
            std::error_code cleanup_error;
            fs::remove(settings_path(), cleanup_error);
        }
#else
        { std::ofstream f(settings_path()); f << settings; }
#endif
        { std::ofstream fc(nick_cache_path()); json jc(nick_copy); fc << jc; }
        { std::ofstream fa(avatar_url_cache_path()); json ja(avatar_copy); fa << ja; }
    } catch (...) {}
}

static void fetch_profile_info(const std::string& id) {
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        if (nick_cache.count(id) && avatar_url_cache.count(id)) return;
    }

    long long steam64 = steam_api::steam3_to_64(id);
    std::string xml = steam_api::fetch_profile_xml(steam64, 6000);
    if (xml.empty()) {
        set_status("profile_failed", id);
        return;
    }

    std::string nick = steam_api::extract_tag(xml, "steamID");
    std::string avatar_url = steam_api::extract_tag(xml, "avatarMedium");
    if (avatar_url.empty()) avatar_url = steam_api::extract_tag(xml, "avatarFull");
    if (avatar_url.empty()) avatar_url = steam_api::extract_tag(xml, "avatarIcon");

    std::lock_guard<std::mutex> lock(g_data_mutex);
    if (!nick.empty())       nick_cache[id] = nick;
    if (!avatar_url.empty()) avatar_url_cache[id] = avatar_url;
}

static void scan_thread() {
    g_scanning = true;
    set_status("scanning");
    save_settings();

    std::vector<std::string> all_ids;
    std::vector<std::string> new_src, new_dst;

    bool scan_failed = false;
    auto scan_dir = [&](const std::string& path, std::vector<std::string>& list) {
        std::error_code ec;
        if (path.empty() || !fs::is_directory(fs::u8path(path), ec) || ec) {
            scan_failed = true;
            set_status("folder_unreadable", path);
            return;
        }

        fs::directory_iterator it(fs::u8path(path), fs::directory_options::skip_permission_denied, ec);
        fs::directory_iterator end;
        if (ec) { scan_failed = true; set_status("folder_unreadable", path); return; }

        for (; it != end; it.increment(ec)) {
            if (ec) break; // stop scanning this folder, keep what we found so far
            bool is_dir = it->is_directory(ec);
            if (ec || !is_dir) continue;
            std::string fname = it->path().filename().u8string();
            if (!fname.empty() &&
                std::all_of(fname.begin(), fname.end(), [](unsigned char c) { return ::isdigit(c); })) {
                all_ids.push_back(fname);
                list.push_back(fname);
            }
        }
    };

    std::string source_path, destination_path;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        source_path = src_path;
        destination_path = dst_path;
    }
    scan_dir(source_path, new_src);
    scan_dir(destination_path, new_dst);

    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        selected_src = -1;
        selected_dst = -1;
        src_list = new_src;
        dst_list = new_dst;
    }

    for (const auto& id : all_ids) {
        if (stop_scan.load()) break;
        fetch_profile_info(id);
    }

    save_settings();
    if (!scan_failed) set_status("scan_complete");
    g_scanning = false;
}

void copy_config() {
    if (g_scanning.load()) return;
    int s_idx = selected_src.load();
    int d_idx = selected_dst.load();

    if (s_idx < 0 || d_idx < 0) { set_status("select_first"); return; }

    std::string s_id, d_id, s_nick, d_nick;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        if (s_idx >= (int)src_list.size() || d_idx >= (int)dst_list.size()) return;
        s_id = src_list[s_idx];
        d_id = dst_list[d_idx];
        s_nick = nick_cache.count(s_id) ? nick_cache[s_id] : ("ID: " + s_id);
        d_nick = nick_cache.count(d_id) ? nick_cache[d_id] : ("ID: " + d_id);
    }

    try {
        fs::path src = fs::u8path(src_path) / s_id / DOTA_ID;
        fs::path dst = fs::u8path(dst_path) / d_id / DOTA_ID;

        if (!fs::exists(src)) { set_status("no_config"); return; }

        std::error_code eq_ec;
        if (fs::exists(dst) && fs::equivalent(src, dst, eq_ec) && !eq_ec) {
            set_status("same_folder");
            return;
        }

        auto retained_backup = replace_config_directory(src, dst);
        if (retained_backup.empty()) set_status("copy_success", d_id);
        else set_status("backup_retained", retained_backup.u8string());

        g_success = {};
        g_success.src_id = s_id;
        g_success.dst_id = d_id;
        g_success.src_nick = s_nick;
        g_success.dst_nick = d_nick;
        g_success.src_folder = src.u8string();
        g_success.dst_folder = dst.u8string();
        g_success.show = true;
    } catch (const ConfigFileError& e) {
        set_status(e.key, e.detail);
    } catch (const std::exception& e) {
        set_status("error", e.what());
    }
}

void start_scan() {
    if (g_scanning.exchange(true)) return;
    if (scan_worker.joinable()) scan_worker.join();
    stop_scan = false;
    scan_worker = std::thread([] {
        try { scan_thread(); }
        catch (const std::exception& e) { set_status("error", e.what()); }
        g_scanning = false;
    });
}

void app_shutdown() {
    stop_scan = true;
    if (scan_worker.joinable()) scan_worker.join();
    save_settings();
}
