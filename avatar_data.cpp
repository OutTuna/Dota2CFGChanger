#include "avatar_data.h"
#include "app.h"
#include <cpr/cpr.h>

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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace fs = std::filesystem;

namespace {

class Semaphore {
public:
    explicit Semaphore(int count) : count_(count) {}
    void acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [&] { return count_ > 0; });
        --count_;
    }
    void release() {
        std::unique_lock<std::mutex> lock(mutex_);
        ++count_;
        cv_.notify_one();
    }
private:
    std::mutex mutex_;
    std::condition_variable cv_;
    int count_;
};

Semaphore g_fetch_slots(8);

const char* USER_AGENT =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36";

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

static void fetch_thread(const std::string steam3_id);
static void mark_failed(const std::string& id);

struct TrackedThread {
    std::thread th;
    std::shared_ptr<std::atomic<bool>> done;
};
static std::mutex g_threads_mutex;
static std::vector<TrackedThread> g_threads;

static void launch_fetch(const std::string& steam3_id) {
    auto done = std::make_shared<std::atomic<bool>>(false);
    std::thread th([steam3_id, done] {
        try { fetch_thread(steam3_id); } catch (...) { mark_failed(steam3_id); }
        done->store(true);
    });

    std::lock_guard<std::mutex> lock(g_threads_mutex);
    g_threads.erase(
        std::remove_if(g_threads.begin(), g_threads.end(),
            [](TrackedThread& t) {
                if (t.done->load()) { t.th.join(); return true; }
                return false;
            }),
        g_threads.end());
    g_threads.push_back({ std::move(th), std::move(done) });
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
}

static fs::path disk_cache_path(const std::string& id) {
    return fs::u8path(config_dir()) / AVATAR_DISK_DIR / (id + ".img");
}

static bool load_from_disk_cache(const std::string& id, std::string& raw_bytes) {
    fs::path p = disk_cache_path(id);
    if (!fs::exists(p)) return false;
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    raw_bytes.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return !raw_bytes.empty();
}

static void save_to_disk_cache(const std::string& id, const std::string& raw_bytes) {
    try {
        fs::create_directories(fs::u8path(config_dir()) / AVATAR_DISK_DIR);
        std::ofstream f(disk_cache_path(id), std::ios::binary);
        if (f) f.write(raw_bytes.data(), (std::streamsize)raw_bytes.size());
    } catch (...) {}
}

static void decode_and_store(const std::string& id, const std::string& raw_bytes) {
    int w = 0, h = 0, ch = 0;
    auto* data = stbi_load_from_memory(
        reinterpret_cast<const unsigned char*>(raw_bytes.data()),
        (int)raw_bytes.size(), &w, &h, &ch, 4);

    if (!data) {
        mark_failed(id);
        return;
    }

    std::vector<unsigned char> pixels(data, data + (size_t)w * h * 4);
    stbi_image_free(data);
    mark_ready(id, std::move(pixels), w, h);
}

static void fetch_thread(const std::string steam3_id) {
    std::string raw;
    if (load_from_disk_cache(steam3_id, raw)) {
        decode_and_store(steam3_id, raw);
        return;
    }

    g_fetch_slots.acquire();
    struct SlotGuard { ~SlotGuard() { g_fetch_slots.release(); } } slot_guard;

    std::string avatar_url;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        auto it = avatar_url_cache.find(steam3_id);
        if (it != avatar_url_cache.end()) avatar_url = it->second;
    }

    if (avatar_url.empty()) {
        long long steam64 = steam_api::steam3_to_64(steam3_id);
        std::string xml = steam_api::fetch_profile_xml(steam64, 6000);
        if (!xml.empty()) {
            avatar_url = steam_api::extract_tag(xml, "avatarMedium");
            if (avatar_url.empty()) avatar_url = steam_api::extract_tag(xml, "avatarFull");
            if (avatar_url.empty()) avatar_url = steam_api::extract_tag(xml, "avatarIcon");
            if (!avatar_url.empty()) {
                std::lock_guard<std::mutex> lock(g_data_mutex);
                avatar_url_cache[steam3_id] = avatar_url;
            }
        }
    }

    if (avatar_url.empty()) {
        mark_failed(steam3_id);
        return;
    }

    auto img = cpr::Get(
        cpr::Url{ avatar_url },
        cpr::Header{{"User-Agent", USER_AGENT}},
        cpr::Timeout{ 8000 },
        cpr::Redirect{ cpr::PostRedirectFlags::POST_ALL });

    if (img.status_code != 200 || img.text.empty()) {
        mark_failed(steam3_id);
        return;
    }

    save_to_disk_cache(steam3_id, img.text);
    decode_and_store(steam3_id, img.text);
}

void avatar_data_request(const std::string& steam3_id) {
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
    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto& [id, entry] : g_avatars) {
        if (entry.state == AvatarState::Ready && !entry.pixels.empty()) {
            result.push_back({id, std::move(entry.pixels), entry.width, entry.height});
            entry.state = AvatarState::Uploaded;
        }
    }
    return result;
}

void avatar_data_shutdown() {
    {
        std::lock_guard<std::mutex> lock(g_threads_mutex);
        for (auto& t : g_threads) {
            if (t.th.joinable()) t.th.join();
        }
        g_threads.clear();
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    g_avatars.clear();
}
