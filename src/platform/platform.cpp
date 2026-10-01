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
    for (const std::string& command : {
        "zenity --file-selection --directory --title=" + quote(title) + " 2>/dev/null",
        "kdialog --getexistingdirectory --title " + quote(title) + " 2>/dev/null"}) {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) continue;
        char buffer[512];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe)) result += buffer;
        int code = pclose(pipe);
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) result.pop_back();
        if (code == 0 && !result.empty()) return result;
        if (code == 256) return {};
    }
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
