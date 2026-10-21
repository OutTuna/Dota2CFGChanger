#pragma once
#include <filesystem>
#include <string>
#include <stdexcept>
#include <functional>
#include <map>
#include <cstdint>

std::string normalize_user_path(const std::string& path);
std::string default_steam_userdata();
void write_file_atomic(const std::filesystem::path& path, const std::string& bytes);
std::filesystem::path replace_config_directory(const std::filesystem::path& source,
    const std::filesystem::path& destination,
    const std::function<void()>& before_replace = {});

struct ConfigFileError : std::runtime_error {
    std::string key;
    std::string detail;
    explicit ConfigFileError(const std::string& message, const std::string& value = {})
        : std::runtime_error(message), key(message), detail(value) {}
};

struct ConfigEntry {
    bool directory = false;
    std::uintmax_t size = 0;
    std::filesystem::file_time_type modified{};
    bool operator==(const ConfigEntry& other) const {
        return directory == other.directory && size == other.size && modified == other.modified;
    }
};
using ConfigTree = std::map<std::filesystem::path, ConfigEntry>;
bool config_path_contains(const std::filesystem::path& parent, const std::filesystem::path& child);
void reject_config_link(const std::filesystem::path& path);
ConfigTree inspect_config_tree(const std::filesystem::path& path);
bool config_files_equal(const std::filesystem::path& left, const std::filesystem::path& right);
void require_config_space(const std::filesystem::path& folder, std::uintmax_t bytes);
