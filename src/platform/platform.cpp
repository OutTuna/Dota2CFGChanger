#include "platform.h"
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <cstdio>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;

std::string exe_dir() {
#ifdef _WIN32
    std::vector<wchar_t> buf(32768);
    DWORD n = GetModuleFileNameW(NULL, buf.data(), static_cast<DWORD>(buf.size()));
    if (n == 0 || n >= buf.size()) return fs::current_path().u8string();
    return fs::path(buf.data()).parent_path().u8string();
#elif defined(__linux__)
    std::error_code ec;
    fs::path self = fs::read_symlink("/proc/self/exe", ec);
    if (ec || self.empty()) return fs::current_path().u8string();
    return self.parent_path().u8string();
#else
    return fs::current_path().u8string();
#endif
}

std::string config_dir() {
    fs::path dir;
#ifdef _WIN32
    const wchar_t* appdata = _wgetenv(L"APPDATA");
    dir = appdata ? (fs::path(appdata) / "DotaManager") : fs::u8path(exe_dir());
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        dir = fs::path(xdg) / "DotaManager";
    } else if (const char* home = std::getenv("HOME")) {
        dir = fs::path(home) / ".config" / "DotaManager";
    } else {
        dir = fs::u8path(exe_dir());
    }
#endif
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir.u8string();
}

std::string browse_for_folder(const char* title, bool* unavailable) {
    if (unavailable) *unavailable = false;
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
                    result = fs::path(path).u8string();
                    CoTaskMemFree(path);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return result;

#elif defined(__linux__)
    auto quote = [](const std::string& value) {
        std::string out = "'";
        for (char c : value) out += c == '\'' ? "'\\''" : std::string(1, c);
        return out + "'";
    };
    bool found = false;
    for (const std::string& command : {
        "zenity --file-selection --directory --title=" + quote(title) + " 2>/dev/null",
        "kdialog --getexistingdirectory --title " + quote(title) + " 2>/dev/null",
        "qarma --file-selection --directory --title=" + quote(title) + " 2>/dev/null",
        "yad --file --directory --title=" + quote(title) + " 2>/dev/null"}) {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) continue;
        char buffer[512];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe)) result += buffer;
        int code = pclose(pipe);
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) result.pop_back();
        if (code == 0 && !result.empty()) return result;
        if (WIFEXITED(code) && WEXITSTATUS(code) == 127) continue;
        found = true;
        if (WIFEXITED(code) && WEXITSTATUS(code) <= 1) return {};
    }
    if (unavailable) *unavailable = !found;
    return {};

#else
    return "";
#endif
}

static std::string find_first_existing(const std::vector<fs::path>& paths) {
    for (const auto& p : paths) {
        std::error_code ec;
        if (fs::exists(p, ec) && fs::is_regular_file(p, ec)) return p.u8string();
    }
    return {};
}

#ifdef __linux__
static FontPaths find_linux_fonts() {
    std::vector<fs::path> roots;
    const char* home = std::getenv("HOME");
    const char* data = std::getenv("XDG_DATA_HOME");
    if (data && *data && fs::u8path(data).is_absolute()) roots.push_back(fs::u8path(data) / "fonts");
    else if (home && *home) roots.push_back(fs::u8path(home) / ".local/share/fonts");
    if (home && *home) roots.push_back(fs::u8path(home) / ".fonts");
    roots.push_back("/usr/local/share/fonts");
    roots.push_back("/usr/share/fonts");
    roots.push_back("/run/current-system/sw/share/X11/fonts");
    std::vector<fs::path> fonts;
    for (const auto& root : roots) {
        std::error_code ec;
        fs::recursive_directory_iterator iterator(root, fs::directory_options::skip_permission_denied, ec), end;
        while (!ec && iterator != end) {
            std::error_code entry_error;
            if (iterator->is_regular_file(entry_error) && !entry_error) fonts.push_back(iterator->path());
            iterator.increment(ec);
        }
    }
    struct Family { const char* regular; const char* bold; };
    for (const Family& family : {
        Family{"JetBrainsMono-Regular.ttf", "JetBrainsMono-Bold.ttf"},
        Family{"JetBrainsMonoNL-Regular.ttf", "JetBrainsMonoNL-Bold.ttf"},
        Family{"JetBrainsMonoNerdFontMono-Regular.ttf", "JetBrainsMonoNerdFontMono-Bold.ttf"},
        Family{"JetBrainsMonoNerdFont-Regular.ttf", "JetBrainsMonoNerdFont-Bold.ttf"},
        Family{"DejaVuSans.ttf", "DejaVuSans-Bold.ttf"},
        Family{"LiberationSans-Regular.ttf", "LiberationSans-Bold.ttf"},
        Family{"NotoSans-Regular.ttf", "NotoSans-Bold.ttf"},
        Family{"AdwaitaSans-Regular.ttf", "AdwaitaSans-Bold.ttf"},
        Family{"OpenSans-Regular.ttf", "OpenSans-Bold.ttf"},
        Family{"Roboto-Regular.ttf", "Roboto-Bold.ttf"},
        Family{"Ubuntu-R.ttf", "Ubuntu-B.ttf"},
        Family{"FreeSans.ttf", "FreeSansBold.ttf"}}) {
        for (const auto& font : fonts) {
            if (font.filename() != family.regular) continue;
            auto bold = find_first_existing({font.parent_path() / family.bold});
            return {font.u8string(), bold.empty() ? font.u8string() : bold};
        }
    }
    return {};
}
#endif

FontPaths find_font_paths() {
#ifdef __linux__
    return find_linux_fonts();
#elif defined(_WIN32)
    const wchar_t* windows = _wgetenv(L"WINDIR");
    const fs::path folder = fs::path(windows && *windows ? windows : L"C:\\Windows") / "Fonts";
    FontPaths result;
    result.regular = find_first_existing({folder / "arial.ttf", folder / "segoeui.ttf", folder / "tahoma.ttf"});
    result.bold = find_first_existing({folder / "arialbd.ttf", folder / "segoeuib.ttf", folder / "arial.ttf"});
    return result;
#else
    return {};
#endif
}

void show_fatal_error(const std::string& detail) {
#ifdef _WIN32
    MessageBoxW(nullptr, fs::u8path(detail).wstring().c_str(), L"Dota 2 CFG Changer", MB_OK | MB_ICONERROR);
#else
    std::fprintf(stderr, "Dota 2 CFG Changer: %s\n", detail.c_str());
    auto child = fork();
    if (child != 0) {
        if (child > 0) waitpid(child, nullptr, 0);
        return;
    }
    auto detached = fork();
    if (detached != 0) _exit(0);
    execlp("zenity", "zenity", "--error", ("--text=" + detail).c_str(), static_cast<char*>(nullptr));
    execlp("kdialog", "kdialog", "--error", detail.c_str(), static_cast<char*>(nullptr));
    _exit(1);
#endif
}

bool open_external(const std::string& value) {
    if (value.empty()) return false;
#ifdef _WIN32
    auto wide = fs::u8path(value).wstring();
    return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", wide.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) > 32;
#else
    auto child = fork();
    if (child < 0) return false;
    if (child == 0) {
        auto detached = fork();
        if (detached < 0) _exit(1);
        if (detached > 0) _exit(0);
#ifdef __APPLE__
        execlp("open", "open", value.c_str(), static_cast<char*>(nullptr));
#else
        execlp("xdg-open", "xdg-open", value.c_str(), static_cast<char*>(nullptr));
#endif
        _exit(1);
    }
    int status = 0;
    if (waitpid(child, &status, 0) < 0) return false;
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
#endif
}

#ifdef _WIN32
std::string read_registry_settings() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\OutTuna\\Dota2CFGChanger", 0,
        KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) return {};
    DWORD type = 0, size = 0;
    auto result = RegQueryValueExW(key, L"Settings", nullptr, &type, nullptr, &size);
    if (result != ERROR_SUCCESS || type != REG_SZ || size < sizeof(wchar_t)
        || size > 131072 || size % sizeof(wchar_t) != 0) {
        RegCloseKey(key);
        return {};
    }
    std::vector<wchar_t> value(size / sizeof(wchar_t) + 1, L'\0');
    result = RegQueryValueExW(key, L"Settings", nullptr, &type,
        reinterpret_cast<BYTE*>(value.data()), &size);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || type != REG_SZ) return {};
    int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), -1,
        nullptr, 0, nullptr, nullptr);
    if (length <= 1) return {};
    std::string settings(length, '\0');
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), -1,
        settings.data(), length, nullptr, nullptr)) return {};
    settings.pop_back();
    return settings;
}

bool write_registry_settings(const std::string& settings) {
    int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, settings.c_str(), -1,
        nullptr, 0);
    if (length <= 1 || length > 65536) return false;
    std::vector<wchar_t> value(length);
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, settings.c_str(), -1,
        value.data(), length)) return false;
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\OutTuna\\Dota2CFGChanger", 0,
        nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) return false;
    auto result = RegSetValueExW(key, L"Settings", 0, REG_SZ,
        reinterpret_cast<const BYTE*>(value.data()), static_cast<DWORD>(value.size() * sizeof(wchar_t)));
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}
#endif
