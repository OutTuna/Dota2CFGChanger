#include "avatar.h"
#include "steam_api.h"
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
#include <condition_variable>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


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
    std::mutex              mutex_;
    std::condition_variable cv_;
    int                     count_;
};
Semaphore g_fetch_slots(4);
}

enum class AvatarState { Idle, Fetching, Ready, Failed };

struct AvatarEntry {
    AvatarState              state   = AvatarState::Idle;
    ImTextureID              texture = (ImTextureID)0;
    std::vector<unsigned char> pixels;
    int                      width   = 0;
    int                      height  = 0;
};

static std::map<std::string, AvatarEntry> g_avatars;
static std::mutex                         g_mutex;

static std::string fetch_avatar_url(const std::string& steam3_id) {
    long long steam64 = steam_api::steam3_to_64(steam3_id);
    std::string xml = steam_api::fetch_profile_xml(steam64, /*timeout_ms=*/3000);
    if (xml.empty()) return {};
    return steam_api::extract_tag(xml, "avatarFull");
}

static void fetch_thread(const std::string steam3_id) {
    g_fetch_slots.acquire();
    struct SlotGuard { ~SlotGuard() { g_fetch_slots.release(); } } slot_guard;

    std::string avatar_url = fetch_avatar_url(steam3_id);
    if (avatar_url.empty()) {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_avatars[steam3_id].state = AvatarState::Failed;
        return;
    }

    auto img = cpr::Get(cpr::Url{ avatar_url }, cpr::Timeout{ 4000 });
    if (img.status_code != 200 || img.text.empty()) {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_avatars[steam3_id].state = AvatarState::Failed;
        return;
    }

    const auto& raw = img.text;
    int w = 0, h = 0, ch = 0;
    auto* data = stbi_load_from_memory(
        reinterpret_cast<const unsigned char*>(raw.data()),
        (int)raw.size(), &w, &h, &ch, 4);

    if (!data) {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_avatars[steam3_id].state = AvatarState::Failed;
        return;
    }

    std::vector<unsigned char> pixels(data, data + w * h * 4);
    stbi_image_free(data);

    std::lock_guard<std::mutex> lock(g_mutex);
    auto& entry   = g_avatars[steam3_id];
    entry.pixels  = std::move(pixels);
    entry.width   = w;
    entry.height  = h;
    entry.state   = AvatarState::Ready;
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

    return (ImTextureID)0;
}

void avatar_flush_pending() {
    std::lock_guard<std::mutex> lock(g_mutex);

    for (auto& [id, entry] : g_avatars) {
        if (entry.state == AvatarState::Ready && !entry.texture && !entry.pixels.empty()) {
            GLuint tex    = upload_texture(entry.pixels, entry.width, entry.height);
            entry.texture = (ImTextureID)(void*)(uintptr_t)tex;
            entry.pixels.clear();
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

