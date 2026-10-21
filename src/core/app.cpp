#include "app.h"
#include "backups.h"
#include "file_ops.h"
#include "localization.h"
#include "ui_events.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <memory>

namespace {
std::thread scan_worker;
std::atomic<bool> stop_scan{false};
std::atomic<bool> stopping_app{false};
unsigned long long preview_generation = 0;
struct ProfileRequest {
    bool fetching = false;
    bool success = false;
    std::chrono::steady_clock::time_point failed_at{};
};
std::map<std::string, ProfileRequest> profiles;
std::mutex profiles_mutex;
std::condition_variable profiles_ready;
}

namespace fs = std::filesystem;
using json = nlohmann::json;

static void set_status(const std::string& s, const std::string& detail = {}) {
    std::lock_guard<std::mutex> lock(g_data_mutex);
    status_msg = s;
    status_detail = detail;
    app_status_changed();
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
        app_ui_changed();
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
        } catch (const std::exception& error) { set_status("settings_load_failed", error.what()); }
    }
    if (fs::exists(nick_cache_path())) {
        try {
            std::ifstream f(nick_cache_path());
            json j; f >> j;
            std::lock_guard<std::mutex> lock(g_data_mutex);
            for (auto& el : j.items()) nick_cache[el.key()] = el.value();
        } catch (const std::exception&) { set_status("profile_cache_invalid", CACHE_FILE); }
    }
    if (fs::exists(avatar_url_cache_path())) {
        try {
            std::ifstream f(avatar_url_cache_path());
            json j; f >> j;
            std::lock_guard<std::mutex> lock(g_data_mutex);
            for (auto& el : j.items()) avatar_url_cache[el.key()] = el.value();
        } catch (const std::exception&) { set_status("profile_cache_invalid", AVATAR_URL_CACHE_FILE); }
    }
    ++g_cache_generation;
    app_ui_changed();
}

void save_settings() {
    static std::mutex save_mutex;
    static std::string saved;
    std::lock_guard<std::mutex> save_lock(save_mutex);
    std::string source_path, destination_path;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        source_path = src_path;
        destination_path = dst_path;
    }
    try {
        auto settings = json{{"src", source_path}, {"dst", destination_path},
            {"theme", g_theme.load()}, {"language", language_code()}}.dump();
        if (settings == saved) return;
#ifdef _WIN32
        if (write_registry_settings(settings)) {
            std::error_code cleanup_error;
            fs::remove(settings_path(), cleanup_error);
        } else throw std::runtime_error("Cannot save registry settings");
#else
        write_file_atomic(settings_path(), settings);
#endif
        saved = std::move(settings);
    } catch (const std::exception& error) { set_status("settings_save_failed", error.what()); }
}

bool save_profile_caches() {
    static std::mutex save_mutex;
    static unsigned long long saved_generation = 0;
    static std::string saved_names, saved_avatars;
    std::lock_guard<std::mutex> save_lock(save_mutex);
    if (saved_generation == g_cache_generation.load()) return true;
    std::map<std::string, std::string> names, avatars;
    unsigned long long generation;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        generation = g_cache_generation.load();
        names = nick_cache;
        avatars = avatar_url_cache;
    }
    try {
        auto names_json = json(names).dump();
        auto avatars_json = json(avatars).dump();
        if (names_json != saved_names) {
            write_file_atomic(nick_cache_path(), names_json);
            saved_names = std::move(names_json);
        }
        if (avatars_json != saved_avatars) {
            write_file_atomic(avatar_url_cache_path(), avatars_json);
            saved_avatars = std::move(avatars_json);
        }
        saved_generation = generation;
        return true;
    } catch (const std::exception& error) { set_status("settings_save_failed", error.what()); return false; }
}

const UiSnapshot& app_ui_snapshot() {
    static UiSnapshot cached;
    if (cached.generation == g_ui_generation.load()) return cached;
    std::vector<std::string> source, destination;
    std::map<std::string, std::string> names;
    std::string status, detail;
    unsigned long long generation, account_generation;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        generation = g_ui_generation.load();
        account_generation = g_account_generation.load();
        if (account_generation != cached.account_generation) {
            source = src_list;
            destination = dst_list;
            names = nick_cache;
        }
        status = status_msg;
        detail = status_detail;
    }
    auto prepare = [&](const std::vector<std::string>& ids) {
        std::vector<AccountView> views;
        views.reserve(ids.size());
        for (const auto& id : ids) {
            const auto found = names.find(id);
            const auto& nickname = found == names.end() ? id : found->second;
            views.push_back({id, nickname + " (" + id + ")"});
        }
        return views;
    };
    if (account_generation != cached.account_generation) {
        cached.source = prepare(source);
        cached.destination = prepare(destination);
        cached.account_generation = account_generation;
    }
    cached.status = std::move(status);
    cached.detail = std::move(detail);
    cached.generation = generation;
    return cached;
}

bool ensure_profile_info(const std::string& id) {
    if (stopping_app.load()) return false;
    std::unique_lock<std::mutex> lock(profiles_mutex);
    auto& request = profiles[id];
    if (request.fetching) {
        profiles_ready.wait(lock, [&] { return !request.fetching || stopping_app.load(); });
        return !stopping_app.load() && request.success;
    }
    {
        std::lock_guard<std::mutex> data_lock(g_data_mutex);
        if (nick_cache.count(id) && avatar_url_cache.count(id)) return true;
    }
    if (request.failed_at != std::chrono::steady_clock::time_point{} &&
        std::chrono::steady_clock::now() - request.failed_at < std::chrono::seconds(30)) return false;
    request.fetching = true;
    lock.unlock();
    bool success = false;
    try {
        auto xml = steam_api::fetch_profile_xml(steam_api::steam3_to_64(id), 6000);
        if (!xml.empty() && !stopping_app.load()) {
            auto nick = steam_api::extract_tag(xml, "steamID");
            auto avatar_url = steam_api::extract_tag(xml, "avatarMedium");
            if (avatar_url.empty()) avatar_url = steam_api::extract_tag(xml, "avatarFull");
            if (avatar_url.empty()) avatar_url = steam_api::extract_tag(xml, "avatarIcon");
            success = !nick.empty() || !avatar_url.empty();
            std::lock_guard<std::mutex> data_lock(g_data_mutex);
            bool names_changed = !nick.empty() && nick_cache[id] != nick;
            bool avatar_changed = !avatar_url.empty() && avatar_url_cache[id] != avatar_url;
            if (names_changed) nick_cache[id] = std::move(nick);
            if (avatar_changed) avatar_url_cache[id] = std::move(avatar_url);
            if (names_changed || avatar_changed) ++g_cache_generation;
            if (names_changed) app_ui_changed();
        }
    } catch (...) {}
    lock.lock();
    request.success = success;
    request.fetching = false;
    request.failed_at = success ? std::chrono::steady_clock::time_point{} : std::chrono::steady_clock::now();
    lock.unlock();
    profiles_ready.notify_all();
    return success;
}

static void scan_thread() {
    g_scanning = true;
    set_status("scanning");

    std::vector<std::string> all_ids;
    std::vector<std::string> new_src, new_dst;

    std::string scan_error;
    int profile_errors = 0;
    auto scan_dir = [&](const std::string& path, std::vector<std::string>& list) {
        std::error_code ec;
        if (path.empty() || !fs::is_directory(fs::u8path(path), ec) || ec) {
            if (scan_error.empty()) scan_error = path;
            return;
        }

        fs::directory_iterator it(fs::u8path(path), ec);
        fs::directory_iterator end;
        if (ec) { if (scan_error.empty()) scan_error = path; return; }

        for (; it != end; it.increment(ec)) {
            if (ec) { if (scan_error.empty()) scan_error = path; break; }
            bool is_dir = it->is_directory(ec);
            if (ec) { if (scan_error.empty()) scan_error = path; ec.clear(); continue; }
            if (stop_scan.load()) break;
            if (!is_dir) continue;
            std::string fname = it->path().filename().u8string();
            if (!fname.empty() &&
                std::all_of(fname.begin(), fname.end(), [](unsigned char c) { return ::isdigit(c); })) {
                if (!steam_api::steam3_to_64(fname)) continue;
                try { reject_config_link(it->path()); }
                catch (const ConfigFileError&) { if (scan_error.empty()) scan_error = it->path().u8string(); continue; }
                all_ids.push_back(fname);
                list.push_back(fname);
            }
        }
        if (ec && scan_error.empty()) scan_error = path;
    };

    std::string source_path, destination_path;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        source_path = src_path;
        destination_path = dst_path;
    }
    scan_dir(source_path, new_src);
    std::error_code equivalent_error;
    bool same_path = source_path == destination_path || fs::equivalent(fs::u8path(source_path), fs::u8path(destination_path), equivalent_error);
    if (same_path) new_dst = new_src;
    else scan_dir(destination_path, new_dst);
    std::sort(all_ids.begin(), all_ids.end());
    all_ids.erase(std::unique(all_ids.begin(), all_ids.end()), all_ids.end());

    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        selected_src = -1;
        selected_dst = -1;
        src_list.swap(new_src);
        dst_list.swap(new_dst);
        app_ui_changed();
    }

    for (const auto& id : all_ids) {
        if (stop_scan.load()) break;
        if (!ensure_profile_info(id)) ++profile_errors;
    }

    bool caches_saved = save_profile_caches();
    if (!stop_scan.load() && caches_saved) {
        if (!scan_error.empty()) set_status("folder_unreadable", scan_error);
        else if (profile_errors) set_status("scan_profile_warnings", std::to_string(profile_errors));
        else set_status("scan_complete");
    }
    g_scanning = false;
    ui_events::request_redraw();
}

void cancel_copy_preview() {
    ++preview_generation;
    g_copy_preview_ready = false;
    g_copy_preview = {};
}

void prepare_copy_preview() {
    cancel_copy_preview();
    const auto request = preview_generation;
    std::filesystem::path source, destination;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        int s = selected_src.load(), d = selected_dst.load();
        if (s < 0 || d < 0 || s >= static_cast<int>(src_list.size()) || d >= static_cast<int>(dst_list.size())) return;
        source = fs::u8path(src_path) / src_list[s] / DOTA_ID;
        destination = fs::u8path(dst_path) / dst_list[d] / DOTA_ID;
    }
    file_tasks_start([source, destination, request]() -> FileTaskCompletion {
        try {
            auto preview = preview_config_transfer(source, destination);
            return [preview = std::move(preview), request]() mutable {
                if (request != preview_generation) return;
                g_copy_preview = std::move(preview);
                g_copy_preview_ready = true;
            };
        } catch (const ConfigFileError& error) { set_status(error.key, error.detail); }
        catch (const std::exception& error) { set_status("error", error.what()); }
        return {};
    });
}

void copy_config(const ConfigPreview* preview) {
    if (g_scanning.load() || file_tasks_busy()) return;
    int s_idx = selected_src.load();
    int d_idx = selected_dst.load();

    if (s_idx < 0 || d_idx < 0) { set_status("select_first"); return; }

    std::string s_id, d_id, s_nick, d_nick, source_path, destination_path;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        if (s_idx >= (int)src_list.size() || d_idx >= (int)dst_list.size()) return;
        s_id = src_list[s_idx];
        d_id = dst_list[d_idx];
        s_nick = nick_cache.count(s_id) ? nick_cache[s_id] : ("ID: " + s_id);
        d_nick = nick_cache.count(d_id) ? nick_cache[d_id] : ("ID: " + d_id);
        source_path = src_path;
        destination_path = dst_path;
    }

    const auto expected = preview ? std::make_shared<ConfigPreview>(*preview) : nullptr;
    if (!file_tasks_start([s_id, d_id, s_nick, d_nick, source_path, destination_path, expected]() -> FileTaskCompletion {
        set_status("working");
        try {
            fs::path src = fs::u8path(source_path) / s_id / DOTA_ID;
            fs::path dst = fs::u8path(destination_path) / d_id / DOTA_ID;

            if (!fs::exists(src)) { set_status("no_config"); return {}; }

            std::error_code eq_ec;
            if (fs::exists(dst) && fs::equivalent(src, dst, eq_ec) && !eq_ec) {
                set_status("same_folder");
                return {};
            }

            auto result = copy_config_with_backup(src, dst, fs::u8path(config_dir()) / "backups", d_id, expected.get());
            if (!result.retained_backup.empty()) set_status("backup_retained", result.retained_backup.u8string());
            else if (!result.cleanup_error.empty()) set_status("backup_cleanup_failed", result.cleanup_error);
            else set_status("copy_success", d_id);

            SuccessPopupState success;
            success.src_id = s_id;
            success.dst_id = d_id;
            success.src_nick = s_nick;
            success.dst_nick = d_nick;
            success.src_label = s_nick + " (ID: " + s_id + ")";
            success.dst_label = d_nick + " (ID: " + d_id + ")";
            success.src_folder = src.u8string();
            success.dst_folder = dst.u8string();
            success.show = true;
            return [success = std::move(success)] { g_success = success; };
        } catch (const ConfigFileError& e) {
            set_status(e.key, e.detail);
        } catch (const std::exception& e) {
            set_status("error", e.what());
        }
        return {};
    })) return;
}

void start_scan() {
    if (stopping_app.load() || file_tasks_busy() || g_scanning.exchange(true)) return;
    if (scan_worker.joinable()) scan_worker.join();
    stop_scan = false;
    try {
        scan_worker = std::thread([] {
            try { scan_thread(); }
            catch (const std::exception& e) { set_status("error", e.what()); }
            g_scanning = false;
            ui_events::request_redraw();
        });
    } catch (const std::exception& error) {
        g_scanning = false;
        set_status("error", error.what());
    }
}

void app_request_stop() {
    {
        std::lock_guard<std::mutex> lock(profiles_mutex);
        stopping_app = true;
    }
    stop_scan = true;
    profiles_ready.notify_all();
    steam_api::cancel_requests();
    file_tasks_request_stop();
}

void app_shutdown() {
    app_request_stop();
    if (scan_worker.joinable()) scan_worker.join();
    file_tasks_shutdown();
    save_settings();
    save_profile_caches();
}
