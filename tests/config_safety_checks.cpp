#include "backups.h"
#include "file_ops.h"
#include "steam_api.h"
#include "platform/update_process.h"
#include <thread>
#include <cassert>
#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <limits>
#ifndef _WIN32
#include <sys/stat.h>
#endif
namespace fs = std::filesystem;
void put(const fs::path& path, const std::string& data) { fs::create_directories(path.parent_path()); std::ofstream(path, std::ios::binary) << data; }
std::string read(const fs::path& path) { std::ifstream f(path); return {std::istreambuf_iterator<char>(f), {}}; }
template<class F> void rejects(F action, const std::string& key = {}) {
    bool failed = false;
    try { action(); } catch (const ConfigFileError& error) { failed = key.empty() || error.key == key; }
    assert(failed);
}
int main(int argc, char** argv) {
    const auto arguments = update_arguments(argc, argv);
    if (arguments.size() == 5 && arguments[1] == "--hold-lock") {
        replace_config_directory(fs::u8path(arguments[2]), fs::u8path(arguments[3]), [&] {
            put(fs::u8path(arguments[4]), "ready");
            for (;;) std::this_thread::sleep_for(std::chrono::milliseconds(50));
        });
        return 1;
    }
#ifdef _WIN32
    assert(config_path_contains(fs::path(L"C:\\Steam\\userdata"), fs::path(L"c:\\steam\\USERDATA\\42")));
#endif
    assert(steam_api::steam3_to_64("4294967295") == 76561202255233023LL);
    for (const auto& id : {"", "0", "-1", "+1", "42junk", " 42", "4294967296", "9223372036854775807", "../42"})
        assert(steam_api::steam3_to_64(id) == 0);
    auto root = fs::temp_directory_path() / ("dota-safety-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    auto source = root / fs::u8path("джерело/42/570");
    auto destination = root / fs::u8path("назначение/43/570");
    auto store = root / "backups";
    put(source / "same", "same"); put(destination / "same", "same");
    put(source / "replace", "new"); put(destination / "replace", "old");
    put(source / "added", "added"); put(destination / "removed", "removed");
    fs::create_directories(source / "empty");
    auto preview = preview_config_transfer(source, destination);
    assert(preview.changes && preview.affected_files == 3 && preview.copy_bytes == 12);
    auto expect = [&](const char* name, ConfigChange change) {
        for (const auto& entry : preview.entries) if (entry.relative == name) { assert(entry.change == change); return; }
        assert(false);
    };
    expect("same", ConfigChange::Unchanged); expect("replace", ConfigChange::Replace);
    expect("added", ConfigChange::Add); expect("removed", ConfigChange::Remove);
    put(destination / "external", "preserve");
    rejects([&] { copy_config_with_backup(source, destination, store, "43", &preview); }, "config_changed");
    assert(read(destination / "external") == "preserve" && !fs::exists(store));
    fs::remove(destination / "external");
    preview = preview_config_transfer(source, destination);
    copy_config_with_backup(source, destination, store, "43", &preview);
    assert(read(destination / "replace") == "new" && !fs::exists(destination / "removed"));
    assert(fs::is_directory(destination / "empty"));
    auto count = list_config_backups(store, "43").size();
    auto modified = fs::last_write_time(destination / "same");
    preview = preview_config_transfer(source, destination);
    assert(!preview.changes && preview.affected_files == 0);
    copy_config_with_backup(source, destination, store, "43", &preview);
    assert(list_config_backups(store, "43").size() == count && fs::last_write_time(destination / "same") == modified);

    put(source / "same", "evil");
    assert(preview_config_transfer(source, destination).changes);
    rejects([&] { copy_config_with_backup(source, destination, store, "43", &preview); }, "config_changed");
    rejects([&] { preview_config_transfer(source, source / "nested"); }, "overlap");
    rejects([&] { require_config_space(root, std::numeric_limits<std::uintmax_t>::max()); }, "space_insufficient");

    std::promise<void> entered, resume;
    auto ready = entered.get_future(); auto proceed = resume.get_future().share();
    auto operation = std::async(std::launch::async, [&] {
        return replace_config_directory(source, destination, [&] { entered.set_value(); assert(proceed.wait_for(std::chrono::seconds(5)) == std::future_status::ready); });
    });
    assert(ready.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    rejects([&] { replace_config_directory(source, destination); }, "operation_locked");
    rejects([&] { copy_config_with_backup(source, destination, store, "43"); }, "operation_locked");
    assert(list_config_backups(store, "43").size() == count);
    resume.set_value(); operation.get();
    auto lock = destination; lock += ".dotamanager_lock"; assert(!fs::exists(lock));
    const auto marker = root / "lock-ready";
    auto child = launch_update_process(fs::absolute(fs::u8path(arguments[0])),
        {"--hold-lock", source.u8string(), destination.u8string(), marker.u8string()});
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!fs::exists(marker)) {
        assert(!update_process_exited(child, 0) && std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    rejects([&] { replace_config_directory(source, destination); }, "operation_locked");
    stop_update_process(child); release_update_process(child);
    assert(fs::is_directory(lock) && read(destination / "same") == "evil");
    rejects([&] { replace_config_directory(source, destination); }, "operation_locked");
    fs::remove(lock); replace_config_directory(source, destination);
#ifndef _WIN32
    fs::create_symlink(source / "same", source / "link");
    rejects([&] { preview_config_transfer(source, destination); }, "symlink"); fs::remove(source / "link");
    fs::create_directory_symlink(destination.parent_path(), root / "account-alias");
    rejects([&] { replace_config_directory(source, root / "account-alias/570"); }, "symlink");
    auto fifo = source / "fifo"; assert(mkfifo(fifo.c_str(), 0600) == 0);
    rejects([&] { replace_config_directory(source, destination); }, "symlink"); fs::remove(fifo);
#endif

    put(source / "conflict", "file"); put(destination / "conflict/old", "old");
    auto conflict = preview_config_transfer(source, destination);
    assert(conflict.changes && conflict.affected_files == 2);
    copy_config_with_backup(source, destination, store, "43", &conflict);
    assert(read(destination / "conflict") == "file");
    fs::remove_all(source); fs::create_directory(source);
    auto empty = preview_config_transfer(source, destination); assert(empty.changes);
    copy_config_with_backup(source, destination, store, "43", &empty);
    assert(fs::is_empty(destination));
    rejects([&] { preview_config_transfer(root / "missing", destination); }, "source_invalid");
    fs::remove_all(root);
    std::cout << "Preview, stale plans, no-op transfers, locks, special files, Unicode and Steam IDs passed\n";
}
