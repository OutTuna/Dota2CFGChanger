#include "avatar_data.h"
#include "app.h"
#include "avatar_transport.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <cassert>
#include <iostream>
#include <algorithm>
namespace fs = std::filesystem;
std::string folder, download_bytes;
std::atomic<int> downloads{0}, finished_downloads{0};
std::atomic<bool> slow_download{false}, downloading{false};
std::string download_avatar(const std::string&, const std::function<bool()>& active) {
    ++downloads;
    downloading = true;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (slow_download && active()) {
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    auto bytes = active() ? download_bytes : std::string{};
    ++finished_downloads;
    return bytes;
}
std::string config_dir() { return folder; }
bool ensure_profile_info(const std::string&) { return false; }
int main(int argc, char** argv) {
    auto root = fs::temp_directory_path() / ("dotamanager-avatars-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    folder = root.u8string();
    fs::create_directories(root / "avatar_cache");
    fs::copy_file(fs::u8path(argv[1]), root / "avatar_cache/42.img");
    { std::ifstream f(fs::u8path(argv[1]), std::ios::binary);
      download_bytes.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()); }
    avatar_data_request("../42");
    avatar_data_request("/42");
    avatar_data_request("42");
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    std::vector<AvatarPixels> ready;
    while (ready.empty()) {
        ready = avatar_data_take_ready();
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    assert(ready.size() == 1 && ready[0].id == "42");
    assert(ready[0].width > 0 && ready[0].height > 0 && ready[0].width <= 64 && ready[0].height <= 64);
    assert(ready[0].pixels.size() == static_cast<std::size_t>(ready[0].width) * ready[0].height * 4);
    assert(avatar_data_take_ready().empty());
    for (int i = 100; i < 112; ++i) {
        auto id = std::to_string(i);
        fs::copy_file(fs::u8path(argv[1]), root / ("avatar_cache/" + id + ".img"));
        for (int repeat = 0; repeat < 3; ++repeat) avatar_data_request(id);
    }
    std::vector<std::string> completed;
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (completed.size() < 12) {
        for (auto& image : avatar_data_take_ready()) {
            assert(std::find(completed.begin(), completed.end(), image.id) == completed.end());
            completed.push_back(image.id);
        }
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    assert(avatar_data_take_ready().empty());
    { std::ofstream f(root / "avatar_cache/43.img"); f << "invalid image"; }
    { std::lock_guard<std::mutex> lock(g_data_mutex); avatar_url_cache["43"] = "test-avatar"; }
    avatar_data_request("43");
    ready.clear();
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (ready.empty()) {
        ready = avatar_data_take_ready();
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(ready.size() == 1 && ready[0].id == "43" && downloads == 1);
    { std::ifstream f(root / "avatar_cache/43.img", std::ios::binary);
      std::string saved(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>{});
      while (saved != download_bytes && std::chrono::steady_clock::now() < deadline) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
          f.close(); f.open(root / "avatar_cache/43.img", std::ios::binary);
          saved.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
      }
      assert(saved == download_bytes); }
    download_bytes = "invalid downloaded image";
    { std::lock_guard<std::mutex> lock(g_data_mutex); avatar_url_cache["45"] = "test-avatar"; }
    avatar_data_request("45");
    while (finished_downloads < 2) {
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    slow_download = true; downloading = false;
    { std::lock_guard<std::mutex> lock(g_data_mutex); avatar_url_cache["44"] = "test-avatar"; }
    avatar_data_request("44");
    while (!downloading) {
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    auto stopped = std::chrono::steady_clock::now();
    avatar_data_request_stop();
    avatar_data_shutdown();
    assert(std::chrono::steady_clock::now() - stopped < std::chrono::seconds(1));
    assert(!fs::exists(root / "avatar_cache/44.img"));
    assert(!fs::exists(root / "avatar_cache/45.img"));
    assert(avatar_data_take_ready().empty());
    fs::remove_all(root);
    std::cout << "Avatar cache and background decoding work without a renderer\n";
}
