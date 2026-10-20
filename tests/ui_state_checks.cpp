#include "app.h"
#include "ui_events.h"
#include <cassert>
#include <thread>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <condition_variable>
#include "localization.h"
#include "file_ops.h"
#include "backups.h"
#include <fstream>
#include <nlohmann/json.hpp>

std::string test_folder;
std::string config_dir() { return test_folder; }
#ifdef _WIN32
std::string test_settings;
std::string read_registry_settings() { return test_settings; }
bool write_registry_settings(const std::string& value) { test_settings = value; return true; }
#endif

namespace steam_api {
std::mutex fetch_mutex;
std::condition_variable fetch_cv;
std::atomic<int> fetch_count{0};
bool fetch_started = false, release_fetch = false, cancelled = false;
std::string fetch_profile_xml(long long id, int) {
    ++fetch_count;
    if (id == steam3_to_64("701")) return {};
    std::unique_lock<std::mutex> lock(fetch_mutex);
    fetch_started = true;
    fetch_cv.notify_all();
    assert(fetch_cv.wait_for(lock, std::chrono::seconds(3), [] { return release_fetch || cancelled; }));
    return cancelled ? "" : "<steamID>Shared profile</steamID><avatarMedium>test-avatar</avatarMedium>";
}
void cancel_requests() {
    std::lock_guard<std::mutex> lock(fetch_mutex);
    cancelled = true;
    fetch_cv.notify_all();
}
}

namespace {
std::atomic<int> wakes{0};
void wake() { ++wakes; }
std::mutex scan_mutex;
std::condition_variable scan_finished;
void scan_wake() {
    std::lock_guard<std::mutex> lock(scan_mutex);
    ui_events::dirty = false;
    scan_finished.notify_all();
}
}

int main() {
    ui_events::set_wake(wake);
    ui_events::dirty = false;
    std::vector<std::thread> workers;
    for (int i = 0; i < 8; ++i)
        workers.emplace_back([] { for (int j = 0; j < 1000; ++j) ui_events::request_redraw(); });
    for (auto& worker : workers) worker.join();
    assert(wakes == 1 && ui_events::dirty);
    ui_events::dirty = false;
    ui_events::request_redraw();
    assert(wakes == 2);
    ui_events::set_wake(nullptr);
    ui_events::dirty = false;
    ui_events::request_redraw();
    assert(wakes == 2);

    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        src_list = {"42"}; dst_list = {"43"};
        nick_cache["42"] = "A nickname longer than a small string buffer";
        status_msg = "scan_complete";
        app_ui_changed();
    }
    const auto& first = app_ui_snapshot();
    assert(first.source[0].label == nick_cache["42"] + " (42)");
    assert(first.destination[0].label == "43 (43)");
    const auto generation = first.generation;
    const auto* label = first.source[0].label.data();
    for (int i = 0; i < 1000; ++i) {
        const auto& same = app_ui_snapshot();
        assert(same.generation == generation && same.source[0].label.data() == label);
    }

    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        status_detail = "status only";
        app_status_changed();
    }
    assert(app_ui_snapshot().source[0].label.data() == label);
    assert(app_ui_snapshot().detail == "status only");
    const auto status_generation = app_ui_snapshot().generation;

    std::atomic<bool> done{false};
    std::thread publisher([&] {
        for (int i = 0; i < 1000; ++i) {
            std::lock_guard<std::mutex> lock(g_data_mutex);
            const auto id = std::to_string(i);
            src_list = {id}; dst_list = {id};
            nick_cache.clear(); nick_cache[id] = id;
            status_detail = id;
            app_ui_changed();
        }
        done = true;
    });
    do {
        const auto& state = app_ui_snapshot();
        assert(state.source.size() == 1 && state.destination.size() == 1);
        if (state.generation != status_generation) {
            assert(state.source[0].id == state.detail);
            assert(state.destination[0].id == state.detail);
            assert(state.source[0].label == state.detail + " (" + state.detail + ")");
        }
    } while (!done.load());
    publisher.join();
    assert(app_ui_snapshot().source[0].id == "999");

    namespace fs = std::filesystem;
    auto root = fs::temp_directory_path() / ("dotamanager-ui-state-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    test_folder = root.u8string();
    fs::create_directories(root / "source/42");
    fs::create_directories(root / "destination/43");
    assert(set_config_path(src_path, (root / "source").u8string()));
    assert(set_config_path(dst_path, (root / "destination").u8string()));
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        nick_cache = {{"42", "Source"}, {"43", "Destination"}};
        avatar_url_cache = {{"42", "cached"}, {"43", "cached"}};
        app_ui_changed();
    }
    set_language("uk");
    save_settings();
    set_language("en");
    assert(set_config_path(src_path, ""));
    load_settings();
    assert(std::string(src_path) == (root / "source").u8string());
    assert(std::string(language_code()) == "uk");
    ui_events::set_wake(scan_wake);
    ui_events::dirty = false;
    start_scan();
    {
        std::unique_lock<std::mutex> lock(scan_mutex);
        assert(scan_finished.wait_for(lock, std::chrono::seconds(3), [] { return !g_scanning.load(); }));
    }
    ui_events::set_wake(nullptr);
    const auto& scanned = app_ui_snapshot();
    assert(scanned.source.size() == 1 && scanned.source[0].label == "Source (42)");
    assert(scanned.destination.size() == 1 && scanned.destination[0].label == "Destination (43)");
    assert(scanned.status == "scan_complete");
    fs::create_directories(root / "source/42/570");
    fs::create_directories(root / "destination/43/570");
    std::ofstream(root / "source/42/570/config.txt") << "new";
    std::ofstream(root / "destination/43/570/config.txt") << "old";
    selected_src = 0; selected_dst = 0;
    prepare_copy_preview(); cancel_copy_preview();
    const auto preview_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (file_tasks_busy()) {
        file_tasks_poll();
        assert(std::chrono::steady_clock::now() < preview_deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(!g_copy_preview_ready && g_copy_preview.entries.empty());
    copy_config();
    assert(file_tasks_busy() && !g_success.show);
    start_scan();
    assert(!g_scanning.load());
    assert(set_config_path(dst_path, (root / "different").u8string()));
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (file_tasks_busy()) {
        file_tasks_poll();
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(g_success.show && fs::equivalent(fs::u8path(g_success.dst_folder), root / "destination/43/570"));
    std::string contents;
    { std::ifstream f(root / "destination/43/570/config.txt"); f >> contents; }
    assert(contents == "new");
    assert(list_config_backups(root / "backups", "43").size() == 1);
    assert(success_popup_animating());
    g_success.anim_t = 0.30f; assert(!success_popup_animating());
    g_success.closing = true; assert(success_popup_animating());
    g_success.show = false; assert(!success_popup_animating());
    auto cache_time = fs::last_write_time(root / CACHE_FILE);
    set_language("en"); save_settings(); save_settings();
    assert(fs::last_write_time(root / CACHE_FILE) == cache_time);
    auto blocked = root / "blocked";
    fs::create_directory(blocked);
    std::ofstream(blocked / "keep") << "preserved";
    bool failed_write = false;
    try { write_file_atomic(blocked, "replacement"); } catch (...) { failed_write = true; }
    assert(failed_write && fs::is_regular_file(blocked / "keep"));
    for (const auto& file : fs::directory_iterator(root))
        assert(file.path().filename().u8string().find(".tmp-") == std::string::npos);

    std::vector<std::thread> profile_workers;
    std::atomic<int> attempting{0};
    for (int i = 0; i < 8; ++i) profile_workers.emplace_back([&] {
        ++attempting;
        assert(ensure_profile_info("700"));
    });
    {
        std::unique_lock<std::mutex> lock(steam_api::fetch_mutex);
        assert(steam_api::fetch_cv.wait_for(lock, std::chrono::seconds(3), [] { return steam_api::fetch_started; }));
    }
    while (attempting != 8) std::this_thread::yield();
    { std::lock_guard<std::mutex> lock(steam_api::fetch_mutex); steam_api::release_fetch = true; }
    steam_api::fetch_cv.notify_all();
    for (auto& thread : profile_workers) thread.join();
    assert(steam_api::fetch_count == 1 && nick_cache["700"] == "Shared profile");

    fs::create_directories(root / "warnings/701");
    assert(set_config_path(src_path, (root / "warnings").u8string()));
    assert(set_config_path(dst_path, (root / "warnings").u8string()));
    ui_events::set_wake(scan_wake); ui_events::dirty = false;
    start_scan();
    {
        std::unique_lock<std::mutex> lock(scan_mutex);
        assert(scan_finished.wait_for(lock, std::chrono::seconds(3), [] { return !g_scanning.load(); }));
    }
    assert(app_ui_snapshot().status == "scan_profile_warnings" && app_ui_snapshot().detail == "1");
    assert(steam_api::fetch_count == 2);
    ui_events::set_wake(nullptr);

    { std::lock_guard<std::mutex> lock(steam_api::fetch_mutex);
      steam_api::release_fetch = false; steam_api::fetch_started = false; }
    std::thread pending([] { assert(!ensure_profile_info("702")); });
    {
        std::unique_lock<std::mutex> lock(steam_api::fetch_mutex);
        assert(steam_api::fetch_cv.wait_for(lock, std::chrono::seconds(3), [] { return steam_api::fetch_started; }));
    }
    std::thread follower([] { assert(!ensure_profile_info("702")); });
    app_request_stop();
    pending.join(); follower.join();
    app_shutdown();
    { std::ifstream f(root / AVATAR_URL_CACHE_FILE); nlohmann::json value; f >> value; assert(value["700"] == "test-avatar"); }
    fs::remove_all(root);
    std::cout << "Redraw coalescing, wake lifecycle and coherent cached UI state passed\n";
}
