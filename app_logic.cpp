#include "app_logic.h"
#include "app_state.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;

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

void load_settings() {
    if (fs::exists(SETTINGS_FILE)) {
        try {
            std::ifstream f(SETTINGS_FILE);
            json j; f >> j;
            std::string s = j.value("src", "");
            std::string d = j.value("dst", "");
            strncpy(src_path, s.c_str(), sizeof(src_path)); src_path[sizeof(src_path)-1] = 0;
            strncpy(dst_path, d.c_str(), sizeof(dst_path)); dst_path[sizeof(dst_path)-1] = 0;
        } catch (...) {}
    }
    if (fs::exists(CACHE_FILE)) {
        try {
            std::ifstream f(CACHE_FILE);
            json j; f >> j;
            for (auto& el : j.items()) nick_cache[el.key()] = el.value();
        } catch (...) {}
    }
}

void save_settings() {
    { std::ofstream f(SETTINGS_FILE); json j = {{"src", src_path},{"dst", dst_path}}; f << j; }
    { std::ofstream fc(CACHE_FILE);   json jc(nick_cache); fc << jc; }
}

static std::string clean_xml_nick(std::string raw) {
    for (auto& tag : { std::string("<![CDATA["), std::string("]]>") }) {
        size_t p = raw.find(tag);
        if (p != std::string::npos) raw.replace(p, tag.length(), "");
    }
    return raw;
}

static std::string fetch_nick(const std::string& id) {
    if (nick_cache.count(id)) return nick_cache[id];
    try {
        long long steam64 = std::stoll(id) + 76561197960265728LL;
        auto r = cpr::Get(cpr::Url{"https://steamcommunity.com/profiles/" +
                          std::to_string(steam64) + "?xml=1"}, cpr::Timeout{2000});
        if (r.status_code == 200) {
            size_t s = r.text.find("<steamID>"), e = r.text.find("</steamID>");
            if (s != std::string::npos && e != std::string::npos) {
                std::string nick = clean_xml_nick(r.text.substr(s + 9, e - s - 9));
                nick_cache[id] = nick;
                return nick;
            }
        }
    } catch (...) {}
    return "";
}

void scan_thread() {
    status_msg = "Scanning...";
    save_settings();
    src_list.clear(); dst_list.clear();
    std::vector<std::string> all_ids;

    auto scan_dir = [&](const std::string& path, std::vector<std::string>& list) {
        if (!fs::exists(path)) return;
        for (const auto& entry : fs::directory_iterator(path)) {
            if (!entry.is_directory()) continue;
            std::string fname = entry.path().filename().string();
            if (std::all_of(fname.begin(), fname.end(), ::isdigit)) {
                all_ids.push_back(fname);
                list.push_back(fname);
            }
        }
    };

    scan_dir(src_path, src_list);
    scan_dir(dst_path, dst_list);
    for (const auto& id : all_ids)
        if (!nick_cache.count(id)) fetch_nick(id);
    save_settings();
    status_msg = "Scan Complete!";
}

void copy_config() {
    if (selected_src < 0 || selected_dst < 0) { status_msg = "Select folders first!"; return; }
    if (selected_src >= (int)src_list.size() || selected_dst >= (int)dst_list.size()) return;

    std::string s_id = src_list[selected_src];
    std::string d_id = dst_list[selected_dst];
    fs::path src = fs::path(src_path) / s_id / DOTA_ID;
    fs::path dst = fs::path(dst_path) / d_id / DOTA_ID;

    if (!fs::exists(src)) { status_msg = "No Dota config in source!"; return; }
    try {
        if (fs::exists(dst)) fs::remove_all(dst);
        fs::copy(src, dst, fs::copy_options::recursive);
        status_msg = "Success! Copied to " + d_id;
        g_success = {};
        g_success.src_id     = s_id;
        g_success.dst_id     = d_id;
        g_success.src_nick   = nick_cache.count(s_id) ? nick_cache[s_id] : ("ID: " + s_id);
        g_success.dst_nick   = nick_cache.count(d_id) ? nick_cache[d_id] : ("ID: " + d_id);
        g_success.src_folder = src.string();
        g_success.dst_folder = dst.string();
        g_success.show       = true;
    } catch (std::exception& e) {
        status_msg = "Error: " + std::string(e.what());
    }
}