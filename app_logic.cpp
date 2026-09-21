#include "app_logic.h"
#include "app_state.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <mutex>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#else
#include <array>
#include <cstdio>
#include <cstdlib>
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;

static const char* USER_AGENT =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36";

#ifdef _WIN32
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
#else
static bool command_exists(const char* cmd) {
    std::string check = std::string("command -v ") + cmd + " >/dev/null 2>&1";
    return std::system(check.c_str()) == 0;
}

static std::string run_and_capture(const std::string& cmd) {
    std::array<char, 4096> buf{};
    std::string out;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return out;
    size_t n;
    while ((n = fread(buf.data(), 1, buf.size(), pipe)) > 0)
        out.append(buf.data(), n);
    pclose(pipe);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
        out.pop_back();
    return out;
}

static std::string shell_quote(const std::string& s) {
    std::string q = "'";
    for (char c : s) {
        if (c == '\'') q += "'\\''";
        else q += c;
    }
    q += "'";
    return q;
}

std::string browse_for_folder(const char* title) {
    std::string t = title ? title : "";

    if (command_exists("zenity")) {
        std::string cmd = "zenity --file-selection --directory --title=" +
                           shell_quote(t) + " 2>/dev/null";
        return run_and_capture(cmd);
    }
    if (command_exists("kdialog")) {
        std::string cmd = "kdialog --getexistingdirectory \"$HOME\" --title=" +
                           shell_quote(t) + " 2>/dev/null";
        return run_and_capture(cmd);
    }
    return {};
}

static std::string default_steam_userdata_path() {
    const char* home_env = std::getenv("HOME");
    if (!home_env || !*home_env) return {};
    fs::path home(home_env);

    const fs::path candidates[] = {
        home / ".local/share/Steam/userdata",
        home / ".steam/steam/userdata",
        home / ".steam/root/userdata",
        home / ".var/app/com.valvesoftware.Steam/.local/share/Steam/userdata",
        home / ".steam/debian-installation/userdata",
    };

    for (const auto& c : candidates) {
        std::error_code ec;
        if (fs::exists(c, ec) && fs::is_directory(c, ec)) return c.string();
    }
    return candidates[0].string();
}
#endif

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

#ifndef _WIN32
    if (src_path[0] == '\0' || dst_path[0] == '\0') {
        std::string guess = default_steam_userdata_path();
        if (!guess.empty()) {
            if (src_path[0] == '\0') { strncpy(src_path, guess.c_str(), sizeof(src_path)); src_path[sizeof(src_path)-1] = 0; }
            if (dst_path[0] == '\0') { strncpy(dst_path, guess.c_str(), sizeof(dst_path)); dst_path[sizeof(dst_path)-1] = 0; }
        }
    }
#endif
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
