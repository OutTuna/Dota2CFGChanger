#include "update_install.h"
#include "checksum.h"
#include "platform/update_process.h"
#include <nlohmann/json.hpp>
#include <cassert>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;
void write(const fs::path& path, const std::string& value) { std::ofstream stream(path, std::ios::binary); stream << value; }
std::string read(const fs::path& path) { std::ifstream stream(path, std::ios::binary); return {std::istreambuf_iterator<char>(stream), {}}; }
ReleaseInfo info(const fs::path& file) {
    ReleaseInfo release;
    release.size = fs::file_size(file);
    release.digest = file_sha256(file);
    return release;
}

int main(int argc, char** argv) {
    auto arguments = update_arguments(argc, argv);
    if (arguments.size() == 3 && arguments[1] == "--apply-update") return update_helper_dispatch(arguments);
    if (arguments.size() == 3 && arguments[1] == "--update-started") {
        update_install_confirm_started(arguments);
        write(fs::u8path(arguments[2]).parent_path() / "child-opened", "started");
        std::this_thread::sleep_for(std::chrono::seconds(3));
        return 0;
    }
    if (arguments.size() == 3 && arguments[1] == "--test-parent") {
        auto folder = fs::u8path(arguments[2]);
        auto target = folder / "application.exe";
        auto staged = target; staged += ".update-new";
        auto release = info(staged);
        auto job = folder / "job.json";
        nlohmann::json data = {{"parent", current_process_id()}, {"target", target.u8string()},
            {"staged", staged.u8string()}, {"digest", release.digest}, {"size", release.size}};
        write(job, data.dump());
        auto helper = launch_update_process(fs::absolute(fs::u8path(arguments[0])), {"--apply-update", job.u8string()});
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (!fs::exists(folder / "ready")) {
            assert(!update_process_exited(helper, 0));
            assert(std::chrono::steady_clock::now() < deadline);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        release_update_process(helper);
        return 0;
    }
    auto root = fs::temp_directory_path() / ("dotamanager-install-" + std::to_string(current_process_id()));
    fs::remove_all(root);
    fs::create_directories(root / fs::u8path("path with spaces and кириллица"));
    auto target = root / fs::u8path("path with spaces and кириллица") / "application.exe";
    auto staged = target; staged += ".update-new";
    auto backup = target; backup += ".update-old";
    write(target, "old"); write(staged, "new");
    auto release = info(staged);
    replace_update_executable(staged, target, release, [&] { assert(read(target) == "new"); assert(read(backup) == "old"); return true; });
    assert(read(target) == "new" && !fs::exists(backup) && !fs::exists(staged));
    write(target, "old"); write(staged, "new");
    bool failed = false;
    try { replace_update_executable(staged, target, release, [] { return false; }); }
    catch (const std::exception&) { failed = true; }
    assert(failed && read(target) == "old" && !fs::exists(backup));
    write(staged, "bad"); failed = false;
    try { replace_update_executable(staged, target, release, [] { return true; }); }
    catch (const std::exception&) { failed = true; }
    assert(failed && read(target) == "old");
    write(staged, "new"); write(backup, "saved"); failed = false;
    try { replace_update_executable(staged, target, release, [] { return true; }); }
    catch (const std::exception&) { failed = true; }
    assert(failed && read(target) == "old" && read(backup) == "saved");
    fs::remove(backup); fs::remove(staged);
    auto executable = fs::canonical(fs::u8path(arguments[0]));
    fs::copy_file(executable, staged);
    auto parent = launch_update_process(executable, {"--test-parent", target.parent_path().u8string()});
    assert(update_process_exited(parent, 15000));
    release_update_process(parent);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    while (!fs::exists(target.parent_path() / "child-opened") || fs::exists(backup) || fs::exists(target.parent_path() / "job.json")) {
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    assert(file_sha256(target) == file_sha256(executable));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    fs::remove_all(root);
    std::cout << "Executable replacement, rollback, corrupt updates and helper restart passed\n";
}
