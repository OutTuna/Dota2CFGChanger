#include "updates.h"
#include "update_install.h"
#include "update_transport.h"
#include <nlohmann/json.hpp>
#include <cassert>
#include <thread>
#include <chrono>
#include <atomic>
#include <iostream>

std::string release_body;
std::atomic<int> downloads{0}, installs{0};
std::string config_dir() { assert(false); return {}; }
std::string fetch_update_release(const std::function<bool()>&) { return release_body; }
void download_update_asset(const ReleaseInfo&, const std::filesystem::path&,
    const std::function<bool(std::uint64_t)>&) { ++downloads; }
void prepare_update_install(const std::filesystem::path&, const ReleaseInfo&,
    const std::function<bool()>&) { ++installs; }

UpdateSnapshot wait_checked() {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (updates_snapshot().phase == UpdatePhase::Checking) {
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return updates_snapshot();
}

int main() {
    using json = nlohmann::json;
    json assets = json::array();
    for (const char* name : {"DotaManager.exe", "Dota2_CFG_Changer-x86_64.AppImage"})
        assets.push_back({{"name",name},{"state","uploaded"},{"size",3},
            {"digest","sha256:ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
            {"browser_download_url", std::string(REPOSITORY_URL)+"/releases/download/latest/"+name}});
    json release = {{"draft",false},{"tag_name","latest"},{"name","Latest Build (v1.10)"},{"assets",assets}};
    release_body = release.dump();
    assert(!updates_self_install_enabled());
    updates_check(true);
    auto snapshot = wait_checked();
    assert(snapshot.phase == UpdatePhase::Available && snapshot.popup && !snapshot.release.download_url.empty());
    updates_download(); updates_install();
    assert(updates_snapshot().phase == UpdatePhase::Available);
    assert(downloads == 0 && installs == 0 && !updates_should_exit());
    release["name"] = "Latest Build (v1.9)"; release_body=release.dump();
    updates_check(false); snapshot=wait_checked();
    assert(snapshot.phase == UpdatePhase::Current && !snapshot.popup);
    updates_shutdown();
    std::cout << "Package-managed updates report versions without downloading or replacing files\n";
}
