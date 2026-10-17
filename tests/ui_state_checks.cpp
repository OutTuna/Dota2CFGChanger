#include "app.h"
#include "ui_events.h"
#include <cassert>
#include <thread>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <condition_variable>
#include "localization.h"

std::string test_folder;
std::string config_dir() { return test_folder; }
#ifdef _WIN32
std::string test_settings;
std::string read_registry_settings() { return test_settings; }
bool write_registry_settings(const std::string& value) { test_settings = value; return true; }
#endif

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
        if (state.generation != generation) {
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
    app_shutdown();
    ui_events::set_wake(nullptr);
    const auto& scanned = app_ui_snapshot();
    assert(scanned.source.size() == 1 && scanned.source[0].label == "Source (42)");
    assert(scanned.destination.size() == 1 && scanned.destination[0].label == "Destination (43)");
    assert(scanned.status == "scan_complete");
    fs::remove_all(root);
    std::cout << "Redraw coalescing, wake lifecycle and coherent cached UI state passed\n";
}
