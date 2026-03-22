#include "avatar.h"
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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ─── Internal state ───────────────────────────────────────────────────────────

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

// ─── Helpers ──────────────────────────────────────────────────────────────────

static std::string strip_cdata(std::string s) {
    const std::string open  = "<![CDATA[";
    const std::string close = "]]>";
    size_t p = s.find(open);
    if (p != std::string::npos) s.replace(p, open.size(), "");
    p = s.find(close);
    if (p != std::string::npos) s.replace(p, close.size(), "");
    while (!s.empty() && (s.front() == ' ' || s.front() == '\n' || s.front() == '\r')) s.erase(s.begin());
    while (!s.empty() && (s.back()  == ' ' || s.back()  == '\n' || s.back()  == '\r')) s.pop_back();
    return s;
}

static std::string fetch_avatar_url(const std::string& steam3_id) {
    try {
        long long steam64 = std::stoll(steam3_id) + 76561197960265728LL;
        std::string url = "https://steamcommunity.com/profiles/"
                        + std::to_string(steam64) + "?xml=1";

        auto r = cpr::Get(cpr::Url{ url }, cpr::Timeout{ 3000 });
        if (r.status_code != 200) return {};

        auto find_tag = [&](const std::string& tag) -> std::string {
            std::string open  = "<" + tag + ">";
            std::string close = "</" + tag + ">";
            size_t s = r.text.find(open);
            size_t e = r.text.find(close);
            if (s == std::string::npos || e == std::string::npos) return {};
            return strip_cdata(r.text.substr(s + open.size(), e - s - open.size()));
        };

        return find_tag("avatarFull");
    } catch (...) {
        return {};
    }
}

static void fetch_thread(const std::string steam3_id) {
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

// ─── Public API ───────────────────────────────────────────────────────────────

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