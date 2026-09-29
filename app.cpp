#include "app.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <cstdlib>

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
            cpr::Header{{"User-Agent", USER_AGENT}, {"Accept", "text/xml,application/xml,*/*"}},
            cpr::Timeout{ timeout_ms },
            cpr::Redirect{ cpr::PostRedirectFlags::POST_ALL });
        if (r.status_code == 200) return r.text;
    } catch (...) {}
    return {};
}

static std::string decode_xml_entities(std::string s) {
    struct Entity { const char* from; char to; };
    // Order matters: &amp; must be decoded last, or "&amp;lt;" would become "<".
    static const Entity entities[] = {
        {"&lt;", '<'}, {"&gt;", '>'}, {"&quot;", '"'}, {"&apos;", '\''}, {"&amp;", '&'},
    };
    for (const auto& e : entities) {
        size_t pos = 0;
        const size_t from_len = std::strlen(e.from);
        while ((pos = s.find(e.from, pos)) != std::string::npos) {
            s.replace(pos, from_len, 1, e.to);
            pos += 1;
        }
    }
    return s;
}

std::string extract_tag(const std::string& xml, const std::string& tag) {
    const std::string open  = "<" + tag + ">";
    const std::string close = "</" + tag + ">";
    size_t s = xml.find(open);
    if (s == std::string::npos) return {};
    size_t e = xml.find(close, s + open.size());
    if (e == std::string::npos) return {};
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

    return decode_xml_entities(raw);
}

}

std::string exe_dir() {
#ifdef _WIN32
    wchar_t buf[MAX_PATH] = {};
    DWORD n = GetModuleFileNameW(NULL, buf, MAX_PATH);
    if (n == 0 || n == MAX_PATH) return fs::current_path().string();
    return fs::path(buf).parent_path().string();
#elif defined(__linux__)
    std::error_code ec;
    fs::path self = fs::read_symlink("/proc/self/exe", ec);
    if (ec || self.empty()) return fs::current_path().string();
    return self.parent_path().string();
#else
    return fs::current_path().string();
#endif
}

std::string config_dir() {
    fs::path dir;
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    dir = appdata ? (fs::path(appdata) / "DotaManager") : fs::path(exe_dir());
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        dir = fs::path(xdg) / "DotaManager";
    } else if (const char* home = std::getenv("HOME")) {
        dir = fs::path(home) / ".config" / "DotaManager";
    } else {
        dir = fs::path(exe_dir());
    }
#endif
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir.string();
}

std::string browse_for_folder(const char* title) {
#ifdef _WIN32
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

#elif defined(__linux__)
    std::string cmd = "zenity --file-selection --directory --title=\"" + std::string(title) + "\" 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    
    if (!pipe) {
        cmd = "kdialog --getexistingdirectory --title=\"" + std::string(title) + "\" 2>/dev/null";
        pipe = popen(cmd.c_str(), "r");
    }
    
    if (!pipe) return "";
    
    char buffer[512];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    
    return result;

#else
    return "";
#endif
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

#ifdef _WIN32
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
#else
    // Common distro paths for fonts that cover Cyrillic (the UI text is in
    // Russian). DejaVu ships with essentially every desktop distro; Liberation
    // and Noto are common fallbacks.
    const std::vector<fs::path> regular_candidates = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
    };

    const std::vector<fs::path> bold_candidates = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
    };
#endif

    result.regular = find_first_existing(regular_candidates);
    result.bold = find_first_existing(bold_candidates);

    return result;
}

static void set_status(const std::string& s) {
    std::lock_guard<std::mutex> lock(g_data_mutex);
    status_msg = s;
}

static fs::path settings_path()  { return fs::path(config_dir()) / SETTINGS_FILE; }
static fs::path nick_cache_path(){ return fs::path(config_dir()) / CACHE_FILE; }
static fs::path avatar_url_cache_path() { return fs::path(config_dir()) / AVATAR_URL_CACHE_FILE; }

void load_settings() {
    if (fs::exists(settings_path())) {
        try {
            std::ifstream f(settings_path());
            json j; f >> j;
            std::string s = j.value("src", "");
            std::string d = j.value("dst", "");
            g_theme = j.value("theme", 0);
            strncpy(src_path, s.c_str(), sizeof(src_path)); src_path[sizeof(src_path)-1] = 0;
            strncpy(dst_path, d.c_str(), sizeof(dst_path)); dst_path[sizeof(dst_path)-1] = 0;
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
    std::map<std::string, std::string> nick_copy;
    std::map<std::string, std::string> avatar_copy;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        nick_copy = nick_cache;
        avatar_copy = avatar_url_cache;
    }
    try {
        { std::ofstream f(settings_path()); json j = {{"src", src_path},{"dst", dst_path},{"theme", g_theme}}; f << j; }
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
        set_status("Profile fetch failed for " + id);
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

void scan_thread() {
    g_scanning = true;
    set_status("Scanning...");
    save_settings();

    std::vector<std::string> all_ids;
    std::vector<std::string> new_src, new_dst;

    auto scan_dir = [&](const std::string& path, std::vector<std::string>& list) {
        std::error_code ec;
        if (path.empty() || !fs::exists(path, ec) || ec) return;

        fs::directory_iterator it(path, fs::directory_options::skip_permission_denied, ec);
        fs::directory_iterator end;
        if (ec) { set_status("Can't read folder: " + path); return; }

        for (; it != end; it.increment(ec)) {
            if (ec) break; // stop scanning this folder, keep what we found so far
            bool is_dir = it->is_directory(ec);
            if (ec || !is_dir) continue;
            std::string fname = it->path().filename().string();
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

    std::error_code eq_ec;
    if (fs::exists(dst) && fs::equivalent(src, dst, eq_ec) && !eq_ec) {
        set_status("Source and destination are the same folder!");
        return;
    }

    // Copy to a temp folder next to dst first, then swap it in. If anything
    // goes wrong mid-copy (disk full, a file locked by Dota, ...) the
    // existing dst is left untouched instead of being half-deleted.
    fs::path tmp = dst;
    tmp += ".dotamanager_tmp";

    try {
        std::error_code ec;
        fs::remove_all(tmp, ec);

        fs::copy(src, tmp, fs::copy_options::recursive);

        if (fs::exists(dst)) fs::remove_all(dst);
        fs::rename(tmp, dst);

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
        std::error_code ec;
        fs::remove_all(tmp, ec);
        set_status("Error: " + std::string(e.what()));
    }
}
