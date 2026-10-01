#pragma once
#include <filesystem>
#include <string>
#include <stdexcept>

std::string normalize_user_path(const std::string& path);
std::string default_steam_userdata();
std::filesystem::path replace_config_directory(const std::filesystem::path& source,
    const std::filesystem::path& destination);

struct ConfigFileError : std::runtime_error {
    std::string key;
    std::string detail;
    explicit ConfigFileError(const std::string& message, const std::string& value = {})
        : std::runtime_error(message), key(message), detail(value) {}
};
