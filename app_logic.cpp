#include "app_logic.h"
#include "app_state.h"
#include "steam_api.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <mutex>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;


static void set_status(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_data_mutex);
    status_msg = msg;
}

static fs::path settings_dir() {
#ifdef _WIN32
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path))) {
        fs::path p = fs::path(path) / "Dota2CFGChanger";
        CoTaskMemFree(path);
        return p;
    }
    return fs::path("Dota2CFGChanger");
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    fs::path base = xdg ? fs::path(xdg)
                        : fs::path(std::getenv("HOME") ? std::getenv("HOME") : ".") / ".config";
    return base / "dota2cfgchanger";
#endif
}

static fs::path settings_file() { return settings_dir() / "settings.json"; }

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
                    char buf[PATH_BUF_SIZE] = {};
                    WideCharToMultiByte(CP_UTF8, 0, path, -1, buf, (int)sizeof(buf), NULL, NULL);
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
    std::error_code ec;
    std::ifstream f(settings_file());
    if (f) {
        try {
            json j; f >> j;

            std::string s = j.value("src", "");
            std::string d = j.value("dst", "");
            strncpy(src_path, s.c_str(), PATH_BUF_SIZE - 1);
            strncpy(dst_path, d.c_str(), PATH_BUF_SIZE - 1);
            src_path[PATH_BUF_SIZE - 1] = 0;
            dst_path[PATH_BUF_SIZE - 1] = 0;

            g_theme = j.value("theme", 0);

            if (j.contains("nicknames") && j["nicknames"].is_object()) {
                std::lock_guard<std::mutex> lock(g_data_mutex);
                for (auto& el : j["nicknames"].items())
                    nick_cache[el.key()] = el.value().get<std::string>();
            }
        } catch (...) {
        }
    }

#ifdef _WIN32
    if (dst_path[0] == '\0')
        strncpy(dst_path, "C:\\Program Files (x86)\\Steam\\userdata", PATH_BUF_SIZE - 1);
#endif
}

void save_settings() {
    json j;
    j["src"]   = src_path;
    j["dst"]   = dst_path;
    j["theme"] = g_theme;

    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        j["nicknames"] = nick_cache;
    }

    std::error_code ec;
    fs::create_directories(settings_dir(), ec);
    fs::path final_path = settings_file();
    fs::path tmp_path    = final_path;
    tmp_path            += ".tmp";

    std::ofstream f(tmp_path, std::ios::trunc);
    if (!f) return;
    f << j.dump(2);
    f.close();

    fs::rename(tmp_path, final_path, ec);
}

static std::string fetch_nick(const std::string& id) {
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        auto it = nick_cache.find(id);
        if (it != nick_cache.end()) return it->second;
    }

    long long steam64 = steam_api::steam3_to_64(id);
    std::string xml = steam_api::fetch_profile_xml(steam64, /*timeout_ms=*/2000);
    if (xml.empty()) return "";

    std::string nick = steam_api::extract_tag(xml, "steamID");
    if (nick.empty()) return "";

    std::lock_guard<std::mutex> lock(g_data_mutex);
    nick_cache[id] = nick;
    return nick;
}

void scan_thread() {
    bool already_scanning = g_scanning.exchange(true);
    if (already_scanning) return;

    set_status("Scanning...");
    save_settings();

    std::vector<std::string> new_src, new_dst, all_ids;

    auto scan_dir = [&](const std::string& path, std::vector<std::string>& list) {
        if (!fs::exists(path)) return;
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(path, ec)) {
            if (ec) break;
            if (!entry.is_directory()) continue;
            std::string fname = entry.path().filename().string();
            if (!fname.empty() && std::all_of(fname.begin(), fname.end(), ::isdigit)) {
                all_ids.push_back(fname);
                list.push_back(fname);
            }
        }
    };

    scan_dir(src_path, new_src);
    scan_dir(dst_path, new_dst);

    for (const auto& id : all_ids)
        fetch_nick(id);
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        src_list = std::move(new_src);
        dst_list = std::move(new_dst);
        selected_src = -1;
        selected_dst = -1;
    }

    save_settings();
    set_status("Scan Complete!");
    g_scanning = false;
}

void copy_config() {
    int sel_src = selected_src.load();
    int sel_dst = selected_dst.load();

    if (sel_src < 0 || sel_dst < 0) {
        set_status("Select folders first!");
        return;
    }

    std::string s_id, d_id;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        if (sel_src >= (int)src_list.size() || sel_dst >= (int)dst_list.size())
            return;
        s_id = src_list[sel_src];
        d_id = dst_list[sel_dst];
    }

    fs::path src = fs::path(src_path) / s_id / DOTA_ID;
    fs::path dst = fs::path(dst_path) / d_id / DOTA_ID;

    if (!fs::exists(src)) { set_status("No Dota config in source!"); return; }

    fs::path dst_tmp = dst; dst_tmp += ".incoming";
    fs::path dst_bak = dst; dst_bak += ".bak";

    std::error_code ec;
    fs::remove_all(dst_tmp, ec);
    fs::remove_all(dst_bak, ec);

    try {
        fs::copy(src, dst_tmp, fs::copy_options::recursive);

        if (fs::exists(dst)) {
            fs::rename(dst, dst_bak, ec);
            if (ec) throw std::runtime_error("could not move aside old config: " + ec.message());
        }

        fs::rename(dst_tmp, dst, ec);
        if (ec) {
            if (fs::exists(dst_bak)) fs::rename(dst_bak, dst, ec);
            throw std::runtime_error("could not finalize copy: " + ec.message());
        }

        fs::remove_all(dst_bak, ec);
        set_status("Success! Copied to " + d_id);

        std::string s_nick, d_nick;
        {
            std::lock_guard<std::mutex> lock(g_data_mutex);
            s_nick = nick_cache.count(s_id) ? nick_cache[s_id] : ("ID: " + s_id);
            d_nick = nick_cache.count(d_id) ? nick_cache[d_id] : ("ID: " + d_id);
        }

        g_success = {};
        g_success.src_id     = s_id;
        g_success.dst_id     = d_id;
        g_success.src_nick   = s_nick;
        g_success.dst_nick   = d_nick;
        g_success.src_folder = src.string();
        g_success.dst_folder = dst.string();
        g_success.show       = true;
    } catch (std::exception& e) {
        fs::remove_all(dst_tmp, ec);
        set_status("Error: " + std::string(e.what()));
    }
}
