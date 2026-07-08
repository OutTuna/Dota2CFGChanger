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
#pragma comment(lib, "advapi32.lib")

#endif

static const char* REG_KEY = "Software\\Dota2CFGChanger";

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
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size;

        size = sizeof(src_path);
        RegQueryValueExA(hKey, "src", NULL, NULL, (LPBYTE)src_path, &size);

        size = sizeof(dst_path);
        RegQueryValueExA(hKey, "dst", NULL, NULL, (LPBYTE)dst_path, &size);

        DWORD theme = 0; size = sizeof(theme);
        if (RegQueryValueExA(hKey, "theme", NULL, NULL, (LPBYTE)&theme, &size) == ERROR_SUCCESS)
            g_theme = (int)theme;

        char nickBuf[65536] = {};
        size = sizeof(nickBuf);
        if (RegQueryValueExA(hKey, "nicknames", NULL, NULL, (LPBYTE)nickBuf, &size) == ERROR_SUCCESS) {
            try {
                json j = json::parse(nickBuf);
                for (auto& el : j.items()) nick_cache[el.key()] = el.value().get<std::string>();
            } catch (...) {}
        }

        RegCloseKey(hKey);
    }

    if (dst_path[0] == '\0')
        strncpy(dst_path, "C:\\Program Files (x86)\\Steam\\userdata", sizeof(dst_path) - 1);
}

void save_settings() {
    HKEY hKey;
    RegCreateKeyExA(HKEY_CURRENT_USER, REG_KEY, 0, NULL,
                    REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);

    RegSetValueExA(hKey, "src",   0, REG_SZ,    (LPBYTE)src_path, (DWORD)strlen(src_path) + 1);
    RegSetValueExA(hKey, "dst",   0, REG_SZ,    (LPBYTE)dst_path, (DWORD)strlen(dst_path) + 1);
    DWORD theme = (DWORD)g_theme;
    RegSetValueExA(hKey, "theme", 0, REG_DWORD, (LPBYTE)&theme, sizeof(theme));

    std::string nickJson = json(nick_cache).dump();
    RegSetValueExA(hKey, "nicknames", 0, REG_SZ,
                   (LPBYTE)nickJson.c_str(), (DWORD)nickJson.size() + 1);

    RegCloseKey(hKey);
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