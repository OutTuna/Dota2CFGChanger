#include "avatar.h"
#include "avatar_data.h"
#include <map>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

static std::map<std::string, ImTextureID> textures;

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


ImTextureID avatar_get(const std::string& id) {
    auto found = textures.find(id);
    if (found != textures.end()) return found->second;
    avatar_data_request(id);
    return (ImTextureID)0;
}

void avatar_flush_pending() {
    for (auto& image : avatar_data_take_ready()) {
        auto texture = upload_texture(image.pixels, image.width, image.height);
        textures[image.id] = (ImTextureID)(void*)(uintptr_t)texture;
    }
}

void avatar_shutdown() {
    avatar_data_shutdown();
    for (auto& [id, texture] : textures) {
        GLuint value = (GLuint)(uintptr_t)(void*)texture;
        glDeleteTextures(1, &value);
    }
    textures.clear();
}
