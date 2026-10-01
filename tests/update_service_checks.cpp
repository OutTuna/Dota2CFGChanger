#include "updates.h"
#include "update_transport.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <cassert>
#include <thread>
#include <chrono>
#include <atomic>
#include <iostream>
namespace fs = std::filesystem;
std::string release_body, folder, download_bytes = "abc";
std::atomic<bool> fetch_failure{false}, slow_download{false};
std::atomic<int> downloads{0};
std::string config_dir() { return folder; }
std::string fetch_update_release(const std::function<bool()>& active) {
    if (!active() || fetch_failure) throw std::runtime_error("Simulated network failure");
    return release_body;
}
void download_update_asset(const ReleaseInfo& release, const fs::path& partial,
    const std::function<bool(std::uint64_t)>& progress) {
    ++downloads;
    std::ofstream f(partial, std::ios::binary); f << download_bytes; f.close();
    while (slow_download) {
        if (!progress(1)) throw std::runtime_error("Cancelled");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    if (!progress(download_bytes.size())) throw std::runtime_error("Cancelled");
}
UpdateSnapshot wait() {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    for (;;) {
        auto value = updates_snapshot();
        if (value.phase != UpdatePhase::Checking && value.phase != UpdatePhase::Downloading) return value;
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
int main() {
    using json = nlohmann::json;
    auto root = fs::temp_directory_path() / ("dotamanager-updates-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    folder = root.u8string();
    json assets = json::array();
    for (const char* name : {"DotaManager.exe", "Dota2_CFG_Changer-x86_64.AppImage"})
        assets.push_back({{"name", name}, {"state", "uploaded"}, {"size", 3}, {"digest", "sha256:ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
            {"browser_download_url", std::string(REPOSITORY_URL) + "/releases/download/latest/" + name}});
    json release = {{"draft", false}, {"tag_name", "latest"}, {"name", "Latest Build (v1.10)"}, {"assets", assets}};
    release_body = release.dump();
    updates_check(false); auto value = wait();
    assert(value.phase == UpdatePhase::Available && value.popup);
    assert(downloads == 0);
    updates_dismiss(); assert(!updates_snapshot().popup);
    updates_download(); value = wait();
    assert(value.phase == UpdatePhase::Downloaded && fs::exists(fs::u8path(value.downloaded_path)));
    assert(downloads == 1);
    auto target = fs::u8path(value.downloaded_path);
    auto partial = fs::u8path(target.u8string() + ".part");
    updates_check(true); wait(); updates_download(); value = wait();
    assert(value.phase == UpdatePhase::Downloaded && downloads == 1);
    { std::ofstream f(target); f << "old"; }
    updates_check(true); wait(); updates_download(); value = wait();
    assert(value.phase == UpdatePhase::Failed && fs::file_size(target) == 3);
    fs::remove(target);
    download_bytes = "bad"; updates_check(true); wait(); updates_download(); value = wait();
    assert(value.phase == UpdatePhase::Failed && !fs::exists(target) && !fs::exists(partial));
    download_bytes = "a"; updates_check(true); wait(); updates_download(); value = wait();
    assert(value.phase == UpdatePhase::Failed && !fs::exists(partial));
    release["name"] = "Latest Build (v1.9)"; release_body = release.dump();
    updates_check(false); value = wait(); assert(value.phase == UpdatePhase::Current && !value.popup);
    updates_check(true); value = wait(); assert(value.phase == UpdatePhase::Current && value.popup);
    updates_dismiss(); fetch_failure = true;
    updates_check(false); value = wait(); assert(value.phase == UpdatePhase::Failed && !value.popup);
    updates_check(true); value = wait(); assert(value.phase == UpdatePhase::Failed && value.popup);
    fetch_failure = false; download_bytes = "abc";
    release["name"] = "Latest Build (v1.10)"; release_body = release.dump();
    updates_check(true); wait(); slow_download = true; updates_download();
    updates_shutdown(); value = updates_snapshot();
    assert(value.phase == UpdatePhase::Failed && !fs::exists(partial));
    fs::remove_all(root);
    std::cout << "Update checks, downloads, corrupt files and cancellation passed\n";
}
