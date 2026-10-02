#pragma once
#include <filesystem>
#include <string>
#include <vector>

struct ConfigBackup {
    std::filesystem::path path;
    std::filesystem::path destination;
    std::string account;
    std::string date;
};
struct BackupCopyResult {
    std::filesystem::path retained_backup;
    std::string cleanup_error;
};
ConfigBackup create_config_backup(const std::filesystem::path& destination,
    const std::filesystem::path& root, const std::string& account);
std::vector<ConfigBackup> list_config_backups(const std::filesystem::path& root,
    const std::string& account);
BackupCopyResult copy_config_with_backup(const std::filesystem::path& source,
    const std::filesystem::path& destination, const std::filesystem::path& root,
    const std::string& account);
BackupCopyResult restore_config_backup(const ConfigBackup& backup,
    const std::filesystem::path& destination, const std::filesystem::path& root,
    const std::string& account);
