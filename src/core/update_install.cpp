#include "update_install.h"
#include "checksum.h"
#include "platform.h"
#include "platform/update_process.h"
#include "json_fields.h"
#include "file_ops.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <iostream>

namespace fs = std::filesystem;
namespace {
fs::path backup_path(const fs::path& target) { auto path = target; path += ".update-old"; return path; }
void write_text(const fs::path& path, const std::string& value) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << value;
    stream.close();
    if (!stream) throw std::runtime_error("Cannot write updater state: " + path.u8string());
}
void verify(const fs::path& path, const ReleaseInfo& release) {
    if (!fs::is_regular_file(fs::symlink_status(path)) || release.digest.empty() ||
        fs::file_size(path) != release.size || file_sha256(path) != release.digest)
        throw std::runtime_error("Update file failed verification");
}
bool wait_ready(const fs::path& path, const UpdateProcess& process, unsigned seconds,
    const std::function<bool()>& active) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    do {
        if (!active() || update_process_exited(process, 0)) return false;
        if (fs::is_regular_file(path)) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}
}

UpdateInstallJob parse_update_install_job(const std::string& body) {
    auto data = nlohmann::json::parse(body);
    UpdateInstallJob job;
    job.target = fs::u8path(data.at("target").get<std::string>());
    job.staged = fs::u8path(data.at("staged").get<std::string>());
    auto expected = job.target; expected += ".update-new";
    if (!job.target.is_absolute() || job.staged != expected) throw std::runtime_error("Invalid updater paths");
    job.release.digest = data.at("digest").get<std::string>();
    job.release.size = json_unsigned(data.at("size"), MAX_UPDATE_BYTES);
    job.parent = json_unsigned(data.at("parent"), maximum_update_process_id());
    if (!job.release.size || !job.parent) throw std::runtime_error("Invalid updater size or parent");
    return job;
}

void cleanup_update_installers() {
    try {
        auto root = fs::u8path(config_dir()) / "updates";
        reject_config_link(root);
        if (!fs::is_directory(fs::symlink_status(root))) return;
        for (const auto& entry : fs::directory_iterator(root)) {
            if (entry.path().filename().u8string().rfind("installer-", 0) != 0 ||
                !fs::is_directory(entry.symlink_status())) continue;
            reject_config_link(entry.path());
            auto marker = entry.path() / "completed";
            if (!fs::is_regular_file(fs::symlink_status(marker))) continue;
            std::ifstream input(marker, std::ios::binary);
            std::string filename;
            std::getline(input, filename);
            input.close();
            auto helper = fs::u8path(filename);
            if (filename.empty() || helper != helper.filename() || helper == "." || helper == ".." || helper == "completed") continue;
            std::error_code error;
            fs::remove(entry.path() / helper, error);
            if (!error) fs::remove_all(entry.path(), error);
        }
    } catch (const std::exception&) {}
}

void replace_update_executable(const fs::path& staged, const fs::path& target,
    const ReleaseInfo& release, const std::function<bool()>& launch) {
    verify(staged, release);
    if (!fs::is_regular_file(fs::symlink_status(target)) || fs::equivalent(staged, target))
        throw std::runtime_error("Invalid application path");
    auto backup = backup_path(target);
    if (fs::exists(fs::symlink_status(backup))) throw std::runtime_error("An earlier update backup must be resolved: " + backup.u8string());
    fs::rename(target, backup);
    try {
        fs::rename(staged, target);
        if (!launch()) throw std::runtime_error("New application did not confirm startup");
    } catch (const std::exception& error) {
        std::string detail = error.what();
        std::error_code ec;
        fs::remove(target, ec);
        if (!ec) fs::rename(backup, target, ec);
        if (ec) throw std::runtime_error(detail + "; recovery failed, old application preserved at " + backup.u8string());
        throw std::runtime_error(detail + "; previous application restored");
    }
    std::error_code ignored;
    fs::remove(backup, ignored);
}

void prepare_update_install(const fs::path& downloaded, const ReleaseInfo& release,
    const std::function<bool()>& active) {
    cleanup_update_installers();
    verify(downloaded, release);
    auto target = running_update_target();
    auto staged = target; staged += ".update-new";
    auto backup = backup_path(target);
    if (fs::exists(fs::symlink_status(staged)) || fs::exists(fs::symlink_status(backup)))
        throw std::runtime_error("An earlier update file must be resolved beside " + target.u8string());
    auto folder = fs::u8path(config_dir()) / "updates" /
        ("installer-" + std::to_string(current_process_id()) + "-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(folder);
#ifndef _WIN32
    fs::permissions(folder, fs::perms::owner_all, fs::perm_options::replace);
#endif
    auto helper = folder / target.filename();
    auto job = folder / "job.json";
    UpdateProcess process;
    bool staged_created = false;
    try {
        fs::copy_file(downloaded, staged, fs::copy_options::none);
        staged_created = true;
        fs::permissions(staged, fs::status(target).permissions());
        verify(staged, release);
        fs::copy_file(target, helper, fs::copy_options::none);
        nlohmann::json data = {{"parent", current_process_id()}, {"target", target.u8string()},
            {"staged", staged.u8string()}, {"digest", release.digest}, {"size", release.size}};
        write_text(job, data.dump());
        if (!active()) throw std::runtime_error("Update installation cancelled");
        process = launch_update_process(helper, {"--apply-update", job.u8string()});
        if (!wait_ready(folder / "ready", process, 15, active))
            throw std::runtime_error("Updater did not become ready; the running application was kept");
        release_update_process(process);
    } catch (...) {
        if (process.id) { stop_update_process(process); release_update_process(process); }
        std::error_code ec;
        if (staged_created) fs::remove(staged, ec);
        fs::remove_all(folder, ec);
        throw;
    }
}

int update_helper_dispatch(const std::vector<std::string>& arguments) {
    if (arguments.size() != 3 || arguments[1] != "--apply-update") return -1;
    fs::path job = fs::u8path(arguments[2]);
    fs::path target;
    UpdateProcess parent;
    bool application_closed = false;
    try {
        std::ifstream input(job);
        if (!input) throw std::runtime_error("Cannot read updater job");
        std::string body{std::istreambuf_iterator<char>(input), {}};
        auto data = parse_update_install_job(body);
        input.close();
        target = data.target;
        const auto& staged = data.staged;
        const auto& release = data.release;
        verify(staged, release);
        parent = watch_update_parent(data.parent);
        write_text(job.parent_path() / "ready", "ready");
        if (!update_process_exited(parent, 60000)) throw std::runtime_error("Application did not close; update not installed");
        release_update_process(parent);
        application_closed = true;
        replace_update_executable(staged, target, release, [&] {
            auto ack = job.parent_path() / "started";
            auto process = launch_update_process(target, {"--update-started", ack.u8string()});
            try {
                bool started = wait_ready(ack, process, 20, [] { return true; });
                if (started) started = !update_process_exited(process, 1000);
                if (!started) stop_update_process(process);
                release_update_process(process);
                return started;
            } catch (...) {
                stop_update_process(process);
                release_update_process(process);
                throw;
            }
        });
        std::error_code ec;
        fs::remove(job, ec);
        fs::remove(job.parent_path() / "ready", ec);
        fs::remove(job.parent_path() / "started", ec);
        try {
            if (job.filename() == "job.json" && job.parent_path().filename().u8string().rfind("installer-", 0) == 0 &&
                fs::weakly_canonical(job.parent_path().parent_path()) == fs::weakly_canonical(fs::u8path(config_dir()) / "updates")) {
                write_text(job.parent_path() / "completed", target.filename().u8string());
                cleanup_update_installers();
            }
        } catch (const std::exception&) {}
        return 0;
    } catch (const std::exception& error) {
        std::string detail = error.what();
        release_update_process(parent);
        try {
            auto log = job.parent_path() / "error.txt";
            write_text(log, detail);
            if (application_closed && !target.empty() && fs::is_regular_file(target) && !fs::exists(backup_path(target))) {
                auto process = launch_update_process(target, {"--update-error", log.u8string()});
                release_update_process(process);
            } else show_update_install_error(detail);
        } catch (...) { show_update_install_error(detail); }
        return 1;
    }
}

void update_install_confirm_started(const std::vector<std::string>& arguments) {
    if (arguments.size() == 3 && arguments[1] == "--update-started")
        write_text(fs::u8path(arguments[2]), "started");
}

std::string update_install_startup_error(const std::vector<std::string>& arguments) {
    if (arguments.size() != 3 || arguments[1] != "--update-error") return {};
    std::ifstream stream(fs::u8path(arguments[2]), std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
