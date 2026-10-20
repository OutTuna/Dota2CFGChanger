#include "avatar_data.h"
#include "app.h"
#include "ui_events.h"
#include "avatar_transport.h"
#include "file_ops.h"

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <memory>
#include <atomic>
#include <deque>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace fs = std::filesystem;

namespace {

const auto RETRY_DELAY = std::chrono::seconds(30);

}

enum class AvatarState { Idle, Fetching, Ready, Uploaded, Failed };

struct AvatarEntry {
    AvatarState state = AvatarState::Idle;
    std::vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    std::chrono::steady_clock::time_point failed_at{};
};

static std::map<std::string, AvatarEntry> g_avatars;
static std::mutex g_mutex;
static std::atomic<bool> g_ready{false};

static void fetch_thread(const std::string steam3_id);
static void mark_failed(const std::string& id);

static std::mutex g_threads_mutex;
static std::condition_variable g_work;
static std::deque<std::string> g_queue;
static std::vector<std::thread> g_threads;
static bool g_stopping = false;
static std::atomic<bool> g_cancelled{false};

static void launch_fetch(const std::string& steam3_id) {
    std::lock_guard<std::mutex> lock(g_threads_mutex);
    if (g_stopping || g_cancelled.load()) return;
    if (g_threads.size() < 8) {
        g_threads.emplace_back([] {
            for (;;) {
                std::string id;
                {
                    std::unique_lock<std::mutex> lock(g_threads_mutex);
                    g_work.wait(lock, [] { return g_stopping || !g_queue.empty(); });
                    if (g_stopping) return;
                    id = std::move(g_queue.front());
                    g_queue.pop_front();
                }
                try { fetch_thread(id); } catch (...) { mark_failed(id); }
            }
        });
    }
    g_queue.push_back(steam3_id);
    g_work.notify_one();
}

static void mark_failed(const std::string& id) {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto& e = g_avatars[id];
    e.state = AvatarState::Failed;
    e.failed_at = std::chrono::steady_clock::now();
}

static void mark_ready(const std::string& id, std::vector<unsigned char>&& pixels, int w, int h) {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto& entry = g_avatars[id];
    entry.pixels = std::move(pixels);
    entry.width = w;
    entry.height = h;
    entry.state = AvatarState::Ready;
    g_ready = true;
    ui_events::request_redraw();
}

static fs::path disk_cache_path(const std::string& id) {
    return fs::u8path(config_dir()) / AVATAR_DISK_DIR / (id + ".img");
}

static bool load_from_disk_cache(const std::string& id, std::string& raw_bytes) {
    fs::path p = disk_cache_path(id);
    std::error_code error;
    if (!fs::exists(p, error) || error) return false;
    if (!fs::is_regular_file(fs::symlink_status(p)) || fs::file_size(p) > MAX_AVATAR_BYTES) return false;
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    raw_bytes.resize(std::min<std::uintmax_t>(fs::file_size(p), MAX_AVATAR_BYTES) + 1);
    f.read(raw_bytes.data(), static_cast<std::streamsize>(raw_bytes.size()));
    if (f.bad() || static_cast<std::size_t>(f.gcount()) > MAX_AVATAR_BYTES) return false;
    raw_bytes.resize(static_cast<std::size_t>(f.gcount()));
    return !raw_bytes.empty();
}

static void save_to_disk_cache(const std::string& id, const std::string& raw_bytes) {
    try {
        fs::create_directories(fs::u8path(config_dir()) / AVATAR_DISK_DIR);
        write_file_atomic(disk_cache_path(id), raw_bytes);
    } catch (...) {}
}

static bool decode_and_store(const std::string& id, const std::string& raw_bytes) {
    if (raw_bytes.empty() || raw_bytes.size() > MAX_AVATAR_BYTES) return false;
    int w = 0, h = 0, ch = 0;
    if (!stbi_info_from_memory(reinterpret_cast<const unsigned char*>(raw_bytes.data()),
        static_cast<int>(raw_bytes.size()), &w, &h, &ch) || w <= 0 || h <= 0 || w > 512 || h > 512)
        return false;
    std::unique_ptr<unsigned char, decltype(&stbi_image_free)> data(stbi_load_from_memory(
        reinterpret_cast<const unsigned char*>(raw_bytes.data()),
        (int)raw_bytes.size(), &w, &h, &ch, 4), stbi_image_free);

    if (!data) {
        return false;
    }

    const int scale = std::max(w, h);
    const int width = scale > 64 ? std::max(1, w * 64 / scale) : w;
    const int height = scale > 64 ? std::max(1, h * 64 / scale) : h;
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4);
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
        auto source = data.get() + (static_cast<std::size_t>(y * h / height) * w + x * w / width) * 4;
        std::copy_n(source, 4, pixels.data() + (static_cast<std::size_t>(y) * width + x) * 4);
    }
    mark_ready(id, std::move(pixels), width, height);
    return true;
}

static void fetch_thread(const std::string steam3_id) {
    std::string raw;
    if (g_cancelled.load()) return;
    if (load_from_disk_cache(steam3_id, raw) && decode_and_store(steam3_id, raw)) return;
    if (g_cancelled.load()) return;

    std::string avatar_url;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        auto it = avatar_url_cache.find(steam3_id);
        if (it != avatar_url_cache.end()) avatar_url = it->second;
    }

    if (avatar_url.empty()) {
        ensure_profile_info(steam3_id);
        std::lock_guard<std::mutex> lock(g_data_mutex);
        auto it = avatar_url_cache.find(steam3_id);
        if (it != avatar_url_cache.end()) avatar_url = it->second;
    }

    if (avatar_url.empty()) {
        mark_failed(steam3_id);
        return;
    }

    auto bytes = download_avatar(avatar_url, [] { return !g_cancelled.load(); });

    if (g_cancelled.load()) return;
    if (bytes.empty() || !decode_and_store(steam3_id, bytes)) {
        mark_failed(steam3_id);
        return;
    }

    save_to_disk_cache(steam3_id, bytes);
}

void avatar_data_request(const std::string& steam3_id) {
    if (g_cancelled.load() || steam3_id.empty() || steam3_id.size() > 10 ||
        steam3_id.find_first_not_of("0123456789") != std::string::npos) return;
    std::lock_guard<std::mutex> lock(g_mutex);

    auto it = g_avatars.find(steam3_id);
    if (it == g_avatars.end()) {
        g_avatars[steam3_id].state = AvatarState::Fetching;
        launch_fetch(steam3_id);
        return;
    }

    if (it->second.state == AvatarState::Failed) {
        auto now = std::chrono::steady_clock::now();
        if (now - it->second.failed_at >= RETRY_DELAY) {
            it->second.state = AvatarState::Fetching;
            it->second.failed_at = now;
            launch_fetch(steam3_id);
        }
    }

    return;
}

std::vector<AvatarPixels> avatar_data_take_ready() {
    std::vector<AvatarPixels> result;
    if (!g_ready.exchange(false)) return result;
    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto& [id, entry] : g_avatars) {
        if (entry.state == AvatarState::Ready && !entry.pixels.empty()) {
            result.push_back({id, std::move(entry.pixels), entry.width, entry.height});
            entry.state = AvatarState::Uploaded;
        }
    }
    return result;
}

void avatar_data_request_stop() {
    g_cancelled = true;
    {
        std::lock_guard<std::mutex> lock(g_threads_mutex);
        g_stopping = true;
        g_queue.clear();
    }
    g_work.notify_all();
}

void avatar_data_shutdown() {
    avatar_data_request_stop();
    for (auto& t : g_threads) if (t.joinable()) t.join();
    g_threads.clear();

    std::lock_guard<std::mutex> lock(g_mutex);
    g_avatars.clear();
    g_ready = false;
    {
        std::lock_guard<std::mutex> threads_lock(g_threads_mutex);
        g_stopping = false;
    }
}
