#include "avatar.h"
#include "app_state.h"
#include <cpr/cpr.h>
#include <backends/imgui_impl_opengl3.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>

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

enum class AvatarState { Idle, Fetching, Ready, Failed };

struct AvatarEntry {
    AvatarState state = AvatarState::Idle;
    ImTextureID texture = (ImTextureID)0;
    std::vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    std::chrono::steady_clock::time_point failed_at{};
};

static std::map<std::string, AvatarEntry> g_avatars;
static std::mutex g_mutex;

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
    return fs::path(AVATAR_DISK_DIR) / (id + ".img");
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
        fs::create_directories(AVATAR_DISK_DIR);
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
        try {
            long long steam64 = std::stoll(steam3_id) + 76561197960265728LL;
            auto r = cpr::Get(
                cpr::Url{"https://steamcommunity.com/profiles/" +
                         std::to_string(steam64) + "?xml=1"},
                cpr::Header{{"User-Agent", USER_AGENT}},
                cpr::Timeout{6000},
                cpr::Redirect{cpr::PostRedirectFlags::POST_ALL});

            if (r.status_code == 200) {
                auto extract = [&](const char* tag) -> std::string {
                    std::string open = std::string("<") + tag + ">";
                    std::string close = std::string("</") + tag + ">";
                    size_t s = r.text.find(open), e = r.text.find(close);
                    if (s == std::string::npos || e == std::string::npos || e <= s) return "";
                    std::string v = r.text.substr(s + open.length(), e - s - open.length());
                    size_t cs = v.find("<![CDATA[");
                    if (cs != std::string::npos) v.erase(cs, 9);
                    size_t ce = v.find("]]>");
                    if (ce != std::string::npos) v.erase(ce, 3);
                    return v;
                };
                avatar_url = extract("avatarMedium");
                if (avatar_url.empty()) avatar_url = extract("avatarFull");
                if (avatar_url.empty()) avatar_url = extract("avatarIcon");
                if (!avatar_url.empty()) {
                    std::lock_guard<std::mutex> lock(g_data_mutex);
                    avatar_url_cache[steam3_id] = avatar_url;
                }
            }
        } catch (...) {}
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

static GLuint upload_texture(const std::vector<unsigned char>& pixels, int w, int h) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

ImTextureID avatar_get(const std::string& steam3_id) {
    std::lock_guard<std::mutex> lock(g_mutex);

    auto it = g_avatars.find(steam3_id);
    if (it == g_avatars.end()) {
        g_avatars[steam3_id].state = AvatarState::Fetching;
        std::thread(fetch_thread, steam3_id).detach();
        return (ImTextureID)0;
    }

    if (it->second.texture)
        return it->second.texture;

    if (it->second.state == AvatarState::Failed) {
        auto now = std::chrono::steady_clock::now();
        if (now - it->second.failed_at >= RETRY_DELAY) {
            it->second.state = AvatarState::Fetching;
            it->second.failed_at = now;
            std::thread(fetch_thread, steam3_id).detach();
        }
    }

    return (ImTextureID)0;
}

void avatar_flush_pending() {
    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto& [id, entry] : g_avatars) {
        if (entry.state == AvatarState::Ready && !entry.texture && !entry.pixels.empty()) {
            GLuint tex = upload_texture(entry.pixels, entry.width, entry.height);
            entry.texture = (ImTextureID)(void*)(uintptr_t)tex;
            entry.pixels.clear();
            entry.pixels.shrink_to_fit();
        }
    }
}

void avatar_shutdown() {
    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto& [id, entry] : g_avatars) {
        if (entry.texture) {
            GLuint tex = (GLuint)(uintptr_t)(void*)entry.texture;
            glDeleteTextures(1, &tex);
            entry.texture = (ImTextureID)0;
        }
    }
    g_avatars.clear();
}
