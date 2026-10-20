#include "file_ops.h"
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <atomic>
#include <chrono>
#include <array>
#include <limits>
#include <algorithm>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

void write_file_atomic(const fs::path& path, const std::string& bytes) {
    static std::atomic<unsigned long long> serial{0};
#ifdef _WIN32
    auto process = GetCurrentProcessId();
#else
    auto process = getpid();
#endif
    auto temporary = path;
    temporary += ".tmp-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
        "-" + std::to_string(process) + "-" + std::to_string(++serial);
    try {
        std::ofstream output;
        output.exceptions(std::ios::failbit | std::ios::badbit);
        output.open(temporary, std::ios::binary | std::ios::trunc);
        output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        output.close();
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Cannot replace saved file");
#else
        fs::rename(temporary, path);
#endif
    } catch (...) {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        throw;
    }
}

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

bool config_path_contains(const fs::path& parent, const fs::path& child) {
    auto p = parent.begin(), c = child.begin();
    for (; p != parent.end() && c != child.end(); ++p, ++c) {
#ifdef _WIN32
        const auto& left = p->native(); const auto& right = c->native();
        if (CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(),
            static_cast<int>(right.size()), TRUE) != CSTR_EQUAL) return false;
#else
        if (*p != *c) return false;
#endif
    }
    return p == parent.end();
}

void reject_config_link(const fs::path& path) {
    if (fs::is_symlink(fs::symlink_status(path))) throw ConfigFileError("symlink");
#ifdef _WIN32
    const auto attributes = GetFileAttributesW(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        throw ConfigFileError("symlink");
#endif
}

ConfigTree inspect_config_tree(const fs::path& path) {
    reject_config_link(path);

    reject_config_link(path.parent_path());
    if (!fs::is_directory(fs::symlink_status(path))) throw ConfigFileError("source_invalid");
    ConfigTree result;
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        reject_config_link(entry.path());
        auto status = entry.symlink_status();
        ConfigEntry value;
        value.directory = fs::is_directory(status);
        if (!value.directory) {
            if (!fs::is_regular_file(status)) throw ConfigFileError("symlink");
            value.size = entry.file_size();
            value.modified = entry.last_write_time();
        }
        result.emplace(entry.path().lexically_relative(path), value);
    }
    return result;
}

bool config_files_equal(const fs::path& left, const fs::path& right) {
    if (fs::file_size(left) != fs::file_size(right)) return false;
    std::ifstream a(left, std::ios::binary), b(right, std::ios::binary);
    if (!a || !b) throw ConfigFileError("source_invalid");
    std::array<char, 65536> x, y;
    do {
        a.read(x.data(), x.size()); b.read(y.data(), y.size());
        if (a.bad() || b.bad()) throw ConfigFileError("source_invalid");
        if (a.gcount() != b.gcount() || !std::equal(x.begin(), x.begin() + a.gcount(), y.begin())) return false;
    } while (a && b);
    return a.eof() && b.eof();
}

void require_config_space(const fs::path& folder, std::uintmax_t bytes) {
    auto existing = fs::absolute(folder);
    while (!fs::exists(existing) && existing.has_relative_path()) existing = existing.parent_path();
    if (fs::space(existing).available < bytes) throw ConfigFileError("space_insufficient");
}

fs::path replace_config_directory(const fs::path& source, const fs::path& destination,
    const std::function<void()>& before_replace) {
    if (!fs::is_directory(source)) throw ConfigFileError("source_invalid");
    const auto src = fs::weakly_canonical(source);
    const auto dst = fs::weakly_canonical(destination);
    if (config_path_contains(src, dst) || config_path_contains(dst, src)) throw ConfigFileError("overlap");
    reject_config_link(source); reject_config_link(source.parent_path());
    reject_config_link(destination); reject_config_link(destination.parent_path());

    auto lock_path = dst; lock_path += ".dotamanager_lock";
    if (!fs::create_directory(lock_path)) throw ConfigFileError("operation_locked");
    struct Lock {
        fs::path path;
        ~Lock() { std::error_code ignored; fs::remove(path, ignored); }
    } lock{lock_path};
    fs::path temp = destination; temp += ".dotamanager_tmp";
    fs::path backup = destination; backup += ".dotamanager_backup";
    if (fs::exists(fs::symlink_status(temp)) || fs::exists(fs::symlink_status(backup)))
        throw ConfigFileError("previous_copy", destination.u8string());
    const auto before = inspect_config_tree(source);
    const bool existed = fs::exists(fs::symlink_status(destination));
    const auto previous = existed ? inspect_config_tree(destination) : ConfigTree{};
    std::uintmax_t bytes = 0;
    for (const auto& item : before) {
        if (item.second.size > std::numeric_limits<std::uintmax_t>::max() - bytes)
            throw ConfigFileError("space_insufficient");
        bytes += item.second.size;
    }
    require_config_space(destination.parent_path(), bytes);
    if (before_replace) before_replace();
    bool saved = false, temp_created = false;
    try {
        if (!fs::create_directory(temp)) throw ConfigFileError("previous_copy", destination.u8string());
        temp_created = true;
        fs::copy(source, temp, fs::copy_options::recursive);
        auto staged = inspect_config_tree(temp);
        if (staged.size() != before.size() || inspect_config_tree(source) != before)
            throw ConfigFileError("config_changed");
        for (const auto& item : before) {
            auto found = staged.find(item.first);
            if (found == staged.end() || found->second.directory != item.second.directory ||
                found->second.size != item.second.size ||
                (!item.second.directory && !config_files_equal(source / item.first, temp / item.first)))
                throw ConfigFileError("config_changed");
        }
        if (inspect_config_tree(source) != before ||
            existed != fs::exists(fs::symlink_status(destination)) ||
            (existed && inspect_config_tree(destination) != previous)) throw ConfigFileError("config_changed");
        if (existed) {
            fs::rename(destination, backup);
            saved = true;
        }
        fs::rename(temp, destination);
    } catch (...) {
        if (saved) {
            std::error_code restore_error;
            fs::rename(backup, destination, restore_error);
            if (restore_error) throw ConfigFileError("restore_failed", backup.u8string());
        }
        std::error_code cleanup_error;
        if (temp_created) fs::remove_all(temp, cleanup_error);
        if (cleanup_error) throw ConfigFileError("staging_cleanup_failed", temp.u8string());
        throw;
    }
    if (saved) {
        std::error_code cleanup_error;
        fs::remove_all(backup, cleanup_error);
        if (cleanup_error) return backup;
    }
    return {};
}
