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
    if (fs::exists(CACHE_FILE)) {
        try {
            std::ifstream f(CACHE_FILE);
            json j; f >> j;
            std::lock_guard<std::mutex> lock(g_data_mutex);
            for (auto& el : j.items()) nick_cache[el.key()] = el.value();
        } catch (...) {}
    }
}

void save_settings() {
    std::map<std::string, std::string> cache_copy;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        cache_copy = nick_cache;
    }
    { std::ofstream f(SETTINGS_FILE); json j = {{"src", src_path},{"dst", dst_path},{"theme", g_theme}}; f << j; }
    { std::ofstream fc(CACHE_FILE); json jc(cache_copy); fc << jc; }
}

static std::string clean_xml_nick(std::string raw) {
    for (auto& tag : { std::string("<![CDATA["), std::string("]]>") }) {
        size_t p = raw.find(tag);
        if (p != std::string::npos) raw.replace(p, tag.length(), "");
    }
    return raw;
}

static std::string fetch_nick(const std::string& id) {
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        if (nick_cache.count(id)) return nick_cache[id];
    }

    std::string nick;
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
            return "";
        }

        size_t s = r.text.find("<steamID>"), e = r.text.find("</steamID>");
        if (s != std::string::npos && e != std::string::npos && e > s) {
            nick = clean_xml_nick(r.text.substr(s + 9, e - s - 9));
        }
    } catch (...) {
        return "";
    }

    if (!nick.empty()) {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        nick_cache[id] = nick;
    }
    return nick;
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
        fetch_nick(id);

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
