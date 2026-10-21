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
#include <vector>
#include <algorithm>
#ifndef _WIN32
#include <sys/stat.h>
#else
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winioctl.h>
#include <sddl.h>
#endif
namespace fs = std::filesystem;
void put(const fs::path& path, const std::string& data) { fs::create_directories(path.parent_path()); std::ofstream(path, std::ios::binary) << data; }
std::string read(const fs::path& path) { std::ifstream f(path); return {std::istreambuf_iterator<char>(f), {}}; }
#ifdef _WIN32
void junction(const fs::path& link, const fs::path& target) {
    fs::create_directory(link);
    auto handle = CreateFileW(link.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    assert(handle != INVALID_HANDLE_VALUE);
    struct MountPoint {
        DWORD tag;
        WORD length, reserved, substitute_offset, substitute_length, print_offset, print_length;
        WCHAR paths[32768];
    } data{};
    const auto printable = fs::absolute(target).make_preferred().native();
    const auto substitute = L"\\??\\" + printable;
    data.tag = IO_REPARSE_TAG_MOUNT_POINT;
    data.substitute_length = static_cast<WORD>(substitute.size() * sizeof(WCHAR));
    data.print_offset = data.substitute_length + sizeof(WCHAR);
    data.print_length = static_cast<WORD>(printable.size() * sizeof(WCHAR));
    std::copy(substitute.begin(), substitute.end(), data.paths);
    std::copy(printable.begin(), printable.end(), data.paths + substitute.size() + 1);
    data.length = 8 + data.print_offset + data.print_length + sizeof(WCHAR);
    DWORD returned = 0;
    const bool created = DeviceIoControl(handle, FSCTL_SET_REPARSE_POINT, &data, data.length + 8,
        nullptr, 0, &returned, nullptr) != FALSE;
    if (!created) std::cerr << "Junction creation failed: " << GetLastError() << '\n';
    CloseHandle(handle);
    assert(created);
}
#endif
template<class F> void rejects(F action, const std::string& key = {}) {
    bool failed = false;
    try { action(); } catch (const ConfigFileError& error) {
        failed = key.empty() || error.key == key;
        if (!failed) std::cerr << "Expected " << key << ", received " << error.key << '\n';
    }
    if (!failed) std::cerr << "Expected rejection: " << key << '\n';
    assert(failed);
}
int main(int argc, char** argv) {
    std::cout << std::unitbuf;
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

    auto previous_time = fs::last_write_time(source / "same");
    put(source / "same", "evil");
    fs::last_write_time(source / "same", previous_time + std::chrono::seconds(2));
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
#ifdef _WIN32
    auto alias = root / "account alias";
    junction(alias, destination.parent_path());
    rejects([&] { replace_config_directory(source, alias / "570"); }, "symlink");
    fs::remove(alias);
    junction(source / "junction", destination);
    rejects([&] { preview_config_transfer(source, destination); }, "symlink");
    fs::remove(source / "junction");
    std::cout << "Windows junctions rejected\n";
    const auto symbolic = source / "symbolic";
    if (CreateSymbolicLinkW(symbolic.c_str(), (destination / "same").c_str(), 2)) {
        rejects([&] { preview_config_transfer(source, destination); }, "symlink");
        fs::remove(symbolic);
        std::cout << "Windows symbolic link rejected\n";
    } else {
        std::cout << "SKIP Windows symbolic link creation: " << GetLastError() << '\n';
    }
    auto held = CreateFileW((source / "same").c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    assert(held != INVALID_HANDLE_VALUE);
    bool denied = false;
    try { replace_config_directory(source, destination); }
    catch (const fs::filesystem_error&) { denied = true; }
    CloseHandle(held);
    assert(denied && read(destination / "same") == "evil" && !fs::exists(lock));
    held = CreateFileW((destination / "same").c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, 0, nullptr);
    assert(held != INVALID_HANDLE_VALUE);
    denied = false;
    try { replace_config_directory(source, destination); }
    catch (const fs::filesystem_error&) { denied = true; }
    CloseHandle(held);
    assert(denied && read(destination / "same") == "evil" && !fs::exists(lock));
    assert(!fs::exists(fs::path(destination.native() + L".dotamanager_tmp")));
    const auto protected_file = source / "same";
    DWORD descriptor_size = 0;
    GetFileSecurityW(protected_file.c_str(), DACL_SECURITY_INFORMATION, nullptr, 0, &descriptor_size);
    std::vector<BYTE> original_security(descriptor_size);
    assert(GetFileSecurityW(protected_file.c_str(), DACL_SECURITY_INFORMATION,
        reinterpret_cast<PSECURITY_DESCRIPTOR>(original_security.data()), descriptor_size, &descriptor_size));
    PSECURITY_DESCRIPTOR restricted = nullptr;
    assert(ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(D;;FR;;;WD)",
        SDDL_REVISION_1, &restricted, nullptr));
    assert(SetFileSecurityW(protected_file.c_str(), DACL_SECURITY_INFORMATION, restricted));
    LocalFree(restricted);
    denied = false;
    try { replace_config_directory(source, destination); }
    catch (const fs::filesystem_error&) { denied = true; }
    const bool restored_acl = SetFileSecurityW(protected_file.c_str(), DACL_SECURITY_INFORMATION,
        reinterpret_cast<PSECURITY_DESCRIPTOR>(original_security.data())) != FALSE;
    assert(restored_acl && denied && read(destination / "same") == "evil" && !fs::exists(lock));
    std::cout << "Windows denied-read ACL preserved destination\n";
    std::cout << "Windows junctions and sharing violations preserved destination\n";
#endif
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
