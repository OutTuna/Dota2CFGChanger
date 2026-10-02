#include "updates.h"
#include "update_install.h"
#include "checksum.h"
#include "platform.h"
#include "update_transport.h"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <stdexcept>

#include "app_version.h"

namespace fs = std::filesystem;
namespace {
std::mutex mutex;
UpdateSnapshot state;
std::thread worker;
std::atomic<bool> stopping{false};
std::atomic<bool> restart_ready{false};

void fail(const char* key, const std::string& detail) {
    std::lock_guard<std::mutex> lock(mutex);
    state.phase = UpdatePhase::Failed;
    state.error_key = key;
    state.detail = detail;
}
}

const char* app_version() { return DOTA_APP_VERSION; }
UpdateSnapshot updates_snapshot() { std::lock_guard<std::mutex> lock(mutex); return state; }
void updates_dismiss() { std::lock_guard<std::mutex> lock(mutex); state.popup = false; }

void updates_check(bool manual) {
    if (!manual && std::string(app_version()) == "0.0") return;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (state.phase == UpdatePhase::Checking || state.phase == UpdatePhase::Downloading || state.phase == UpdatePhase::Installing) {
            if (manual) state.popup = true;
            return;
        }
        state = {};
        state.phase = UpdatePhase::Checking;
        state.popup = manual;
    }
    if (worker.joinable()) worker.join();
    stopping = false;
    worker = std::thread([manual] {
        try {
            auto body = fetch_update_release([] { return !stopping.load(); });
#ifdef _WIN32
            bool windows = true;
#else
            bool windows = false;
#endif
            auto release = parse_release_info(body, windows);
            bool available = newer_version(release.version, app_version());
            std::lock_guard<std::mutex> lock(mutex);
            state.release = std::move(release);
            state.phase = available ? UpdatePhase::Available : UpdatePhase::Current;
            state.popup = manual || available;
        } catch (const std::exception& e) { fail("update_check_failed", e.what()); }
    });
}

void updates_download() {
    ReleaseInfo release;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if ((state.phase != UpdatePhase::Available && state.phase != UpdatePhase::Failed) || state.release.download_url.empty()) return;
        release = state.release;
        state.phase = UpdatePhase::Downloading;
        state.downloaded_path.clear();
        state.error_key.clear();
        state.progress = 0;
        state.popup = true;
    }
    if (worker.joinable()) worker.join();
    stopping = false;
    worker = std::thread([release] {
        fs::path partial;
        try {
            auto folder = fs::u8path(config_dir()) / "updates" / release.version;
            fs::create_directories(folder);
            auto target = folder / release.asset_name;
            partial = target;
            partial += ".part";
            if (fs::exists(target)) {
                if (fs::file_size(target) != release.size || file_sha256(target) != release.digest)
                    throw std::runtime_error("Existing update file does not match release; open GitHub to download manually");
            } else {
                download_update_asset(release, partial, [release](std::uint64_t now) {
                    std::lock_guard<std::mutex> lock(mutex);
                    state.progress = static_cast<float>(static_cast<double>(now) / release.size);
                    return !stopping.load();
                });
                if (fs::file_size(partial) != release.size) throw std::runtime_error("Download incomplete");
                if (file_sha256(partial) != release.digest) throw std::runtime_error("Download checksum mismatch");
#ifndef _WIN32
                fs::permissions(partial, fs::perms::owner_exec, fs::perm_options::add);
#endif
                fs::rename(partial, target);
            }
            std::lock_guard<std::mutex> lock(mutex);
            state.phase = UpdatePhase::Downloaded;
            state.progress = 1;
            state.downloaded_path = target.u8string();
        } catch (const std::exception& e) {
            std::error_code ec;
            if (!partial.empty()) fs::remove(partial, ec);
            fail("update_download_failed", e.what());
        }
    });
}

void updates_shutdown() {
    stopping = true;
    if (worker.joinable()) worker.join();
}

void updates_install() {
    UpdateSnapshot update;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if ((state.phase != UpdatePhase::Downloaded && state.phase != UpdatePhase::Failed) ||
            state.downloaded_path.empty()) return;
        update = state;
        state.phase = UpdatePhase::Installing;
        state.error_key.clear();
        state.popup = true;
    }
    if (worker.joinable()) worker.join();
    stopping = false;
    worker = std::thread([update] {
        try {
            prepare_update_install(fs::u8path(update.downloaded_path), update.release,
                [] { return !stopping.load(); });
            restart_ready = true;
        } catch (const std::exception& error) { fail("update_install_failed", error.what()); }
    });
}

bool updates_should_exit() { return restart_ready.load(); }
void updates_install_error(const std::string& detail) {
    fail("update_install_failed", detail);
    std::lock_guard<std::mutex> lock(mutex);
    state.popup = true;
}
