#include "file_ops.h"
#include <cstdlib>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace fs = std::filesystem;

std::string normalize_user_path(const std::string& path) {
    if (path.empty()) return {};
    std::string value = path;
#ifndef _WIN32
    if (value == "~" || value.rfind("~/", 0) == 0) {
        const char* home = std::getenv("HOME");
        if (!home || !*home) throw ConfigFileError("home_missing");
        value = std::string(home) + value.substr(1);
    }
#endif
    return fs::absolute(fs::u8path(value)).lexically_normal().u8string();
}

std::string default_steam_userdata() {
    std::vector<fs::path> candidates;
#ifdef _WIN32
    wchar_t steam_path[32768] = {};
    DWORD bytes = sizeof(steam_path);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath",
        RRF_RT_REG_SZ, nullptr, steam_path, &bytes) == ERROR_SUCCESS)
        candidates.push_back(fs::path(steam_path) / "userdata");
    const wchar_t* program_files = _wgetenv(L"ProgramFiles(x86)");
    candidates.push_back(fs::path(program_files && *program_files ? program_files : L"C:\\Program Files (x86)") / "Steam" / "userdata");
#else
    const char* home = std::getenv("HOME");
    if (!home || !*home) return {};
    fs::path home_path = fs::u8path(home);
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg && *xdg && fs::u8path(xdg).is_absolute())
        candidates.push_back(fs::u8path(xdg) / "Steam" / "userdata");
    candidates.push_back(home_path / ".local/share/Steam/userdata");
    candidates.push_back(home_path / ".steam/steam/userdata");
    candidates.push_back(home_path / ".steam/root/userdata");
    candidates.push_back(home_path / ".var/app/com.valvesoftware.Steam/.local/share/Steam/userdata");
#endif
    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (fs::is_directory(candidate, ec)) return candidate.u8string();
    }
    return candidates.front().u8string();
}

fs::path replace_config_directory(const fs::path& source, const fs::path& destination) {
    if (!fs::is_directory(source)) throw ConfigFileError("source_invalid");
    const auto src = fs::weakly_canonical(source);
    const auto dst = fs::weakly_canonical(destination);
    auto nested = [](const fs::path& parent, const fs::path& child) {
        auto p = parent.begin(), c = child.begin();
        for (; p != parent.end() && c != child.end() && *p == *c; ++p, ++c) {}
        return p == parent.end();
    };
    if (nested(src, dst) || nested(dst, src))
        throw ConfigFileError("overlap");
    fs::path temp = destination;
    temp += ".dotamanager_tmp";
    fs::path backup = destination;
    backup += ".dotamanager_backup";
    if (fs::exists(fs::symlink_status(temp)) || fs::exists(fs::symlink_status(backup)))
        throw ConfigFileError("previous_copy", destination.u8string());
    for (const auto& entry : fs::recursive_directory_iterator(source))
        if (entry.is_symlink()) throw ConfigFileError("symlink");
    if (fs::is_symlink(fs::symlink_status(source)) || fs::is_symlink(fs::symlink_status(destination)))
        throw ConfigFileError("symlink");
    bool saved = false;
    try {
        fs::copy(source, temp, fs::copy_options::recursive);
        if (fs::exists(destination)) {
            fs::rename(destination, backup);
            saved = true;
        }
        fs::rename(temp, destination);
    } catch (...) {
        if (saved) {
            std::error_code restore_error;
            fs::rename(backup, destination, restore_error);
            if (restore_error)
                throw ConfigFileError("restore_failed", backup.u8string());
        }
        std::error_code cleanup_error;
        fs::remove_all(temp, cleanup_error);
        throw;
    }
    if (saved) {
        std::error_code cleanup_error;
        fs::remove_all(backup, cleanup_error);
        if (cleanup_error) return backup;
    }
    return {};
}
