#include "backups.h"
#include "file_ops.h"
#include <cassert>
#include <fstream>
#include <chrono>
#include <iostream>

namespace fs = std::filesystem;
void write(const fs::path& path, const std::string& text) { fs::create_directories(path.parent_path()); std::ofstream out(path, std::ios::binary); out << text; }
std::string read(const fs::path& path) { std::ifstream in(path, std::ios::binary); return {std::istreambuf_iterator<char>(in), {}}; }
template<class F> void fails(F action) { bool failed = false; try { action(); } catch (const std::exception&) { failed = true; } assert(failed); }

int main() {
    auto root = fs::temp_directory_path() / ("dotamanager-backups-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    auto destination = root / fs::u8path("дані з пробілами") / "123" / "570";
    auto source = root / "source" / "570";
    auto store = root / "backups";
    write(source / "cfg/options.txt", "new");
    write(destination / "cfg/options.txt", "old");
    fs::create_directories(destination / "empty-directory");
    auto result = copy_config_with_backup(source, destination, store, "123");
    assert(result.cleanup_error.empty() && result.retained_backup.empty());
    auto backups = list_config_backups(store, "123");
    assert(backups.size() == 1 && read(backups[0].path / "570/cfg/options.txt") == "old");
    assert(fs::is_directory(backups[0].path / "570/empty-directory"));
    assert(read(destination / "cfg/options.txt") == "new");
    restore_config_backup(backups[0], destination, store, "123");
    assert(read(destination / "cfg/options.txt") == "old");
    backups = list_config_backups(store, "123");
    assert(backups.size() == 2 && read(backups[0].path / "570/cfg/options.txt") == "new");
    write(backups[0].path / "570/cfg/options.txt", "corrupt");
    fails([&] { restore_config_backup(backups[0], destination, store, "123"); });
    assert(read(destination / "cfg/options.txt") == "old" && list_config_backups(store, "123").size() == 2);
    write(backups[0].path / "570/cfg/options.txt", "new");
    fails([&] { restore_config_backup(backups[0], root / "other/123/570", store, "123"); });
    fails([&] { restore_config_backup(backups[0], destination, store, "456"); });
    auto blocked = root / "blocked"; write(blocked, "file");
    fails([&] { copy_config_with_backup(source, destination, blocked, "123"); });
    assert(read(destination / "cfg/options.txt") == "old");
    fails([&] { copy_config_with_backup(source, destination, destination / "backups", "123"); });
    fails([&] { copy_config_with_backup(source, destination, store, "../123"); });
    assert(read(destination / "cfg/options.txt") == "old");
    for (int i = 0; i < 8; ++i) {
        write(source / "cfg/options.txt", "version-" + std::to_string(i));
        copy_config_with_backup(source, destination, store, "123");
    }
    backups = list_config_backups(store, "123");
    assert(backups.size() == 5);
    assert(read(backups.front().path / "570/cfg/options.txt") == "version-6");
    assert(read(backups.back().path / "570/cfg/options.txt") == "version-2");
    auto oldest = backups.back();
    restore_config_backup(oldest, destination, store, "123");
    assert(read(destination / "cfg/options.txt") == "version-2");
    backups = list_config_backups(store, "123");
    assert(backups.size() == 5 && read(backups.front().path / "570/cfg/options.txt") == "version-7");
    auto empty_destination = root / "456/570";
    fs::create_directories(empty_destination.parent_path());
    copy_config_with_backup(source, empty_destination, store, "456");
    assert(read(empty_destination / "cfg/options.txt") == "version-7" && list_config_backups(store, "456").empty());
#ifndef _WIN32
    fs::create_symlink(source / "cfg/options.txt", source / "link");
    fails([&] { copy_config_with_backup(source, destination, store, "123"); });
    assert(read(destination / "cfg/options.txt") == "version-2");
    fs::remove(source / "link");
    fs::create_symlink(destination / "cfg/options.txt", destination / "link");
    fails([&] { copy_config_with_backup(source, destination, store, "123"); });
    assert(read(destination / "cfg/options.txt") == "version-2");
#endif
    fs::remove_all(root);
    std::cout << "Backup creation, integrity, retention, restore and failure protection passed\n";
}
