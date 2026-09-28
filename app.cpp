#include "app.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <mutex>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

namespace fs = std::filesystem;
using json = nlohmann::json;

static const char* USER_AGENT =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36";

namespace steam_api {

static const long long STEAM64_BASE = 76561197960265728LL;

long long steam3_to_64(const std::string& steam3_id) {
    try {
        return std::stoll(steam3_id) + STEAM64_BASE;
    } catch (...) {
        return 0;
    }
}

std::string fetch_profile_xml(long long steam64, int timeout_ms) {
    if (steam64 <= 0) return {};
    try {
        auto r = cpr::Get(
            cpr::Url{ "https://steamcommunity.com/profiles/" + std::to_string(steam64) + "?xml=1" },
            cpr::Timeout{ timeout_ms });
        if (r.status_code == 200) return r.text;
    } catch (...) {}
    return {};
}

std::string extract_tag(const std::string& xml, const std::string& tag) {
    const std::string open  = "<" + tag + ">";
    const std::string close = "</" + tag + ">";
    size_t s = xml.find(open);
    size_t e = xml.find(close);
    if (s == std::string::npos || e == std::string::npos || e < s) return {};
    std::string raw = xml.substr(s + open.size(), e - s - open.size());
    const std::string cdata_open  = "<![CDATA[";
    const std::string cdata_close = "]]>";
    size_t p = raw.find(cdata_open);
    if (p != std::string::npos) raw.replace(p, cdata_open.size(), "");
    p = raw.find(cdata_close);
    if (p != std::string::npos) raw.replace(p, cdata_close.size(), "");
    while (!raw.empty() && (raw.front() == ' ' || raw.front() == '\n' || raw.front() == '\r'))
        raw.erase(raw.begin());
    while (!raw.empty() && (raw.back() == ' ' || raw.back() == '\n' || raw.back() == '\r'))
        raw.pop_back();

    return raw;
}

}

std::string browse_for_folder(const char* title) {
    std::string result;
    IFileOpenDialog* pfd = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
        DWORD opts = 0;
        pfd->GetOptions(&opts);
        pfd->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        wchar_t wtitle[256] = {};
        MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, 256);
        pfd->SetTitle(wtitle);
        if (SUCCEEDED(pfd->Show(NULL))) {
            IShellItem* psi = nullptr;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR path = nullptr;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                    char buf[256] = {};
                    WideCharToMultiByte(CP_UTF8, 0, path, -1, buf, 256, NULL, NULL);
                    result = buf;
                    CoTaskMemFree(path);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return result;
}

static std::string find_first_existing(const std::vector<fs::path>& paths) {
    for (const auto& p : paths) {
        std::error_code ec;
        if (fs::exists(p, ec) && fs::is_regular_file(p, ec)) return p.string();
    }
    return {};
}

FontPaths find_font_paths() {
    FontPaths result;

    const std::vector<fs::path> regular_candidates = {
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\tahoma.ttf",
    };

    const std::vector<fs::path> bold_candidates = {
        "C:\\Windows\\Fonts\\arialbd.ttf",
        "C:\\Windows\\Fonts\\segoeuib.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
    };

    result.regular = find_first_existing(regular_candidates);
    result.bold = find_first_existing(bold_candidates);

    return result;
}

static void set_status(const std::string& s) {
    std::lock_guard<std::mutex> lock(g_data_mutex);
    status_msg = s;
}

void load_settings() {
    if (fs::exists(SETTINGS_FILE)) {
        try {
            std::ifstream f(SETTINGS_FILE);
            json j; f >> j;
            std::string s = j.value("src", "");
            std::string d = j.value("dst", "");
            g_theme = j.value("theme", 0);
            strncpy(src_path, s.c_str(), sizeof(src_path)); src_path[sizeof(src_path)-1] = 0;
            strncpy(dst_path, d.c_str(), sizeof(dst_path)); dst_path[sizeof(dst_path)-1] = 0;
        } catch (...) {}
    }
    if (fs::exists(CACHE_FILE)) {
        try {
            std::ifstream f(CACHE_FILE);
            json j; f >> j;
            std::lock_guard<std::mutex> lock(g_data_mutex);
            for (auto& el : j.items()) nick_cache[el.key()] = el.value();
        } catch (...) {}
    }
    if (fs::exists(AVATAR_URL_CACHE_FILE)) {
        try {
            std::ifstream f(AVATAR_URL_CACHE_FILE);
            json j; f >> j;
            std::lock_guard<std::mutex> lock(g_data_mutex);
            for (auto& el : j.items()) avatar_url_cache[el.key()] = el.value();
        } catch (...) {}
    }
}

void save_settings() {
    std::map<std::string, std::string> nick_copy;
    std::map<std::string, std::string> avatar_copy;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        nick_copy = nick_cache;
        avatar_copy = avatar_url_cache;
    }
    { std::ofstream f(SETTINGS_FILE); json j = {{"src", src_path},{"dst", dst_path},{"theme", g_theme}}; f << j; }
    { std::ofstream fc(CACHE_FILE); json jc(nick_copy); fc << jc; }
    { std::ofstream fa(AVATAR_URL_CACHE_FILE); json ja(avatar_copy); fa << ja; }
}

static std::string clean_xml_value(std::string raw) {
    for (auto& tag : { std::string("<![CDATA["), std::string("]]>") }) {
        size_t p = raw.find(tag);
        if (p != std::string::npos) raw.replace(p, tag.length(), "");
    }
    return raw;
}

static std::string extract_xml_tag(const std::string& xml, const std::string& tag) {
    std::string open = "<" + tag + ">";
    std::string close = "</" + tag + ">";
    size_t s = xml.find(open);
    size_t e = xml.find(close);
    if (s == std::string::npos || e == std::string::npos || e <= s) return "";
    return clean_xml_value(xml.substr(s + open.length(), e - s - open.length()));
}

static void fetch_profile_info(const std::string& id) {
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        if (nick_cache.count(id) && avatar_url_cache.count(id)) return;
    }

    try {
        long long steam64 = std::stoll(id) + 76561197960265728LL;

        auto r = cpr::Get(
            cpr::Url{"https://steamcommunity.com/profiles/" +
                     std::to_string(steam64) + "?xml=1"},
            cpr::Header{{"User-Agent", USER_AGENT},
                        {"Accept", "text/xml,application/xml,*/*"}},
            cpr::Timeout{6000},
            cpr::Redirect{cpr::PostRedirectFlags::POST_ALL});

        if (r.status_code != 200) {
            set_status("HTTP " + std::to_string(r.status_code) +
                       (r.error.message.empty() ? "" : (": " + r.error.message)));
            return;
        }

        std::string nick = extract_xml_tag(r.text, "steamID");
        std::string avatar_url = extract_xml_tag(r.text, "avatarMedium");
        if (avatar_url.empty()) avatar_url = extract_xml_tag(r.text, "avatarFull");
        if (avatar_url.empty()) avatar_url = extract_xml_tag(r.text, "avatarIcon");

        std::lock_guard<std::mutex> lock(g_data_mutex);
        if (!nick.empty())       nick_cache[id] = nick;
        if (!avatar_url.empty()) avatar_url_cache[id] = avatar_url;
    } catch (...) {}
}

void scan_thread() {
    g_scanning = true;
    set_status("Scanning...");
    save_settings();

    std::vector<std::string> all_ids;
    std::vector<std::string> new_src, new_dst;

    auto scan_dir = [&](const std::string& path, std::vector<std::string>& list) {
        if (path.empty() || !fs::exists(path)) return;
        for (const auto& entry : fs::directory_iterator(path)) {
            if (!entry.is_directory()) continue;
            std::string fname = entry.path().filename().string();
            if (!fname.empty() &&
                std::all_of(fname.begin(), fname.end(), [](unsigned char c) { return ::isdigit(c); })) {
                all_ids.push_back(fname);
                list.push_back(fname);
            }
        }
    };

    scan_dir(src_path, new_src);
    scan_dir(dst_path, new_dst);

    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        src_list = new_src;
        dst_list = new_dst;
    }

    for (const auto& id : all_ids)
        fetch_profile_info(id);

    save_settings();
    set_status("Scan Complete!");
    g_scanning = false;
}

void copy_config() {
    int s_idx = selected_src.load();
    int d_idx = selected_dst.load();

    if (s_idx < 0 || d_idx < 0) { set_status("Select folders first!"); return; }

    std::string s_id, d_id, s_nick, d_nick;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        if (s_idx >= (int)src_list.size() || d_idx >= (int)dst_list.size()) return;
        s_id = src_list[s_idx];
        d_id = dst_list[d_idx];
        s_nick = nick_cache.count(s_id) ? nick_cache[s_id] : ("ID: " + s_id);
        d_nick = nick_cache.count(d_id) ? nick_cache[d_id] : ("ID: " + d_id);
    }

    fs::path src = fs::path(src_path) / s_id / DOTA_ID;
    fs::path dst = fs::path(dst_path) / d_id / DOTA_ID;

    if (!fs::exists(src)) { set_status("No Dota config in source!"); return; }

    try {
        if (fs::exists(dst)) fs::remove_all(dst);
        fs::copy(src, dst, fs::copy_options::recursive);

        set_status("Success! Copied to " + d_id);

        g_success = {};
        g_success.src_id = s_id;
        g_success.dst_id = d_id;
        g_success.src_nick = s_nick;
        g_success.dst_nick = d_nick;
        g_success.src_folder = src.string();
        g_success.dst_folder = dst.string();
        g_success.show = true;
    } catch (std::exception& e) {
        set_status("Error: " + std::string(e.what()));
    }
}