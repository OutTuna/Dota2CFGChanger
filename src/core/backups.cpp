#include "backups.h"
#include "file_ops.h"
#include "checksum.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace fs = std::filesystem;
using json = nlohmann::json;
namespace {
void check_account(const std::string& account) {
    if (account.empty() || account.find_first_not_of("0123456789") != std::string::npos)
        throw ConfigFileError("backup_invalid");
}
bool contains(const fs::path& parent, const fs::path& child) {
    auto p = parent.begin(), c = child.begin();
    for (; p != parent.end() && c != child.end() && *p == *c; ++p, ++c) {}
    return p == parent.end();
}
void separate(const fs::path& a, const fs::path& b) {
    auto left = fs::weakly_canonical(a), right = fs::weakly_canonical(b);
    if (contains(left, right) || contains(right, left)) throw ConfigFileError("overlap");
}
void check_store(const fs::path& root, const std::string& account) {
    check_account(account);
    if (fs::is_symlink(fs::symlink_status(root)) || fs::is_symlink(fs::symlink_status(root / account)))
        throw ConfigFileError("symlink");
}
json tree(const fs::path& path) {
    if (!fs::is_directory(fs::symlink_status(path))) throw ConfigFileError("backup_invalid");
    json result = json::object();
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        auto status = entry.symlink_status();
        auto name = entry.path().lexically_relative(path).generic_u8string();
        if (fs::is_directory(status)) result[name] = {{"directory", true}};
        else if (fs::is_regular_file(status))
            result[name] = {{"size", entry.file_size()}, {"sha256", file_sha256(entry.path())}};
        else throw ConfigFileError("symlink");
    }
    return result;
}
json metadata(const fs::path& folder) {
    if (!fs::is_directory(fs::symlink_status(folder)) ||
        !fs::is_regular_file(fs::symlink_status(folder / "backup.json"))) throw ConfigFileError("backup_invalid");
    std::ifstream stream(folder / "backup.json");
    json value; stream >> value;
    if (value.at("format").get<int>() != 1) throw ConfigFileError("backup_invalid");
    return value;
}
std::string timestamp(bool filename) {
    auto now = std::chrono::system_clock::now();
    auto raw = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &raw);
#else
    gmtime_r(&raw, &utc);
#endif
    std::ostringstream value;
    value << std::put_time(&utc, filename ? "%Y%m%dT%H%M%S" : "%Y-%m-%d %H:%M:%S UTC");
    if (filename) value << '-' << std::chrono::steady_clock::now().time_since_epoch().count();
    return value.str();
}
std::string prune(const fs::path& root, const std::string& account) {
    try {
        auto backups = list_config_backups(root, account);
        for (std::size_t i = 5; i < backups.size(); ++i) fs::remove_all(backups[i].path);
        return {};
    } catch (const std::exception& error) { return error.what(); }
}
}

ConfigBackup create_config_backup(const fs::path& destination, const fs::path& root,
    const std::string& account) {
    fs::path pending;
    bool created = false;
    try {
        check_store(root, account);
        separate(destination, root);
        auto before = tree(destination);
        auto folder = root / account;
        fs::create_directories(folder);
        auto id = timestamp(true);
        pending = folder / ("pending-" + id);
        if (!fs::create_directory(pending)) throw ConfigFileError("backup_invalid");
        created = true;
        fs::copy(destination, pending / "570", fs::copy_options::recursive);
        if (tree(pending / "570") != before || tree(destination) != before)
            throw ConfigFileError("backup_invalid");
        auto date = timestamp(false);
        json value = {{"format", 1}, {"account", account}, {"date", date},
            {"destination", fs::weakly_canonical(destination).u8string()}, {"files", before}};
        std::ofstream stream(pending / "backup.json", std::ios::binary);
        stream << value.dump(2); stream.close();
        if (!stream) throw ConfigFileError("backup_invalid");
        auto final = folder / id;
        fs::rename(pending, final);
        return {final, fs::weakly_canonical(destination), account, date};
    } catch (const std::exception& error) {
        std::error_code ignored;
        if (created) fs::remove_all(pending, ignored);
        throw ConfigFileError("backup_create_failed", error.what());
    }
}

std::vector<ConfigBackup> list_config_backups(const fs::path& root, const std::string& account) {
    check_store(root, account);
    auto folder = root / account;
    if (!fs::exists(folder)) return {};
    std::vector<ConfigBackup> result;
    for (const auto& entry : fs::directory_iterator(folder)) {
        if (entry.path().filename().u8string().rfind("pending-", 0) == 0) continue;
        try {
            auto value = metadata(entry.path());
            if (value.at("account").get<std::string>() != account) continue;
            auto destination = fs::u8path(value.at("destination").get<std::string>());
            if (!destination.is_absolute()) continue;
            result.push_back({entry.path(), destination, account, value.at("date").get<std::string>()});
        } catch (const std::exception&) {}
    }
    std::sort(result.begin(), result.end(), [](const ConfigBackup& a, const ConfigBackup& b) { return a.path.filename() > b.path.filename(); });
    return result;
}

BackupCopyResult copy_config_with_backup(const fs::path& source, const fs::path& destination,
    const fs::path& root, const std::string& account) {
    check_store(root, account);
    separate(source, destination);
    separate(source, root);
    separate(destination, root);
    tree(source);
    auto temp = destination; temp += ".dotamanager_tmp";
    auto recovery = destination; recovery += ".dotamanager_backup";
    if (fs::exists(fs::symlink_status(temp)) || fs::exists(fs::symlink_status(recovery)))
        throw ConfigFileError("previous_copy", destination.u8string());
    if (fs::exists(fs::symlink_status(destination))) create_config_backup(destination, root, account);
    BackupCopyResult result;
    result.retained_backup = replace_config_directory(source, destination);
    result.cleanup_error = prune(root, account);
    return result;
}

BackupCopyResult restore_config_backup(const ConfigBackup& backup, const fs::path& destination,
    const fs::path& root, const std::string& account) {
    check_store(root, account);
    if (backup.account != account || fs::weakly_canonical(backup.path.parent_path()) != fs::weakly_canonical(root / account))
        throw ConfigFileError("backup_invalid");
    auto value = metadata(backup.path);
    if (value.at("account").get<std::string>() != account ||
        fs::weakly_canonical(fs::u8path(value.at("destination").get<std::string>())) != fs::weakly_canonical(destination) ||
        tree(backup.path / "570") != value.at("files")) throw ConfigFileError("backup_invalid");
    separate(destination, root);
    if (fs::exists(fs::symlink_status(destination))) create_config_backup(destination, root, account);
    BackupCopyResult result;
    result.retained_backup = replace_config_directory(backup.path / "570", destination);
    result.cleanup_error = prune(root, account);
    return result;
}
