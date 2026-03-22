#include "ui.h"
#include "app_state.h"
#include "app_logic.h"
#include "avatar.h"
#include "bg_crimson.h"
#include "backends/imgui_impl_glfw.h"
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>

#include <thread>
#include <cmath>

static AppTheme    g_current_theme    = AppTheme::Dark;
static bool        g_settings_open    = false;
static bool        g_palette_open     = false;
static ImTextureID g_bg_crimson_tex   = (ImTextureID)0;

static ImVec4 g_crimson_child_bg      = { 0.12f, 0.05f, 0.05f, 1.00f };
static ImVec4 g_crimson_selected_bg   = { 0.55f, 0.08f, 0.08f, 1.00f };
static ImVec4 g_crimson_hovered_bg    = { 0.30f, 0.06f, 0.06f, 1.00f };
static ImVec4 g_crimson_text          = { 0.95f, 0.88f, 0.88f, 1.00f };



static void apply_dark(ImGuiStyle& st) {
    st.WindowRounding    = 8.f;
    st.ChildRounding     = 6.f;
    st.FrameRounding     = 5.f;
    st.PopupRounding     = 6.f;
    st.ScrollbarRounding = 5.f;
    st.GrabRounding      = 4.f;
    st.TabRounding       = 5.f;
    st.WindowBorderSize  = 1.f;
    st.FrameBorderSize   = 0.f;
    st.WindowPadding     = { 14.f, 14.f };
    st.FramePadding      = { 10.f,  6.f };
    st.ItemSpacing       = {  8.f,  5.f };
    st.ItemInnerSpacing  = {  6.f,  3.f };
    st.ScrollbarSize     = 10.f;
    st.GrabMinSize       = 8.f;
    st.IndentSpacing     = 18.f;

    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg]             = { 0.09f, 0.09f, 0.10f, 1.00f };
    c[ImGuiCol_ChildBg]              = { 0.11f, 0.11f, 0.13f, 1.00f };
    c[ImGuiCol_PopupBg]              = { 0.10f, 0.10f, 0.12f, 1.00f };
    c[ImGuiCol_Border]               = { 0.22f, 0.22f, 0.26f, 1.00f };
    c[ImGuiCol_BorderShadow]         = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_FrameBg]              = { 0.15f, 0.15f, 0.18f, 1.00f };
    c[ImGuiCol_FrameBgHovered]       = { 0.28f, 0.28f, 0.32f, 1.00f };
    c[ImGuiCol_FrameBgActive]        = { 0.24f, 0.24f, 0.28f, 1.00f };
    c[ImGuiCol_TitleBg]              = { 0.07f, 0.07f, 0.08f, 1.00f };
    c[ImGuiCol_TitleBgActive]        = { 0.07f, 0.07f, 0.08f, 1.00f };
    c[ImGuiCol_TitleBgCollapsed]     = { 0.07f, 0.07f, 0.08f, 1.00f };
    c[ImGuiCol_ScrollbarBg]          = { 0.09f, 0.09f, 0.10f, 1.00f };
    c[ImGuiCol_ScrollbarGrab]        = { 0.28f, 0.28f, 0.32f, 1.00f };
    c[ImGuiCol_ScrollbarGrabHovered] = { 0.38f, 0.38f, 0.44f, 1.00f };
    c[ImGuiCol_ScrollbarGrabActive]  = { 0.50f, 0.50f, 0.58f, 1.00f };
    c[ImGuiCol_CheckMark]            = { 0.90f, 0.25f, 0.25f, 1.00f };
    c[ImGuiCol_SliderGrab]           = { 0.80f, 0.22f, 0.22f, 1.00f };
    c[ImGuiCol_SliderGrabActive]     = { 1.00f, 0.30f, 0.30f, 1.00f };
    c[ImGuiCol_Button]               = { 0.20f, 0.20f, 0.24f, 1.00f };
    c[ImGuiCol_ButtonHovered]        = { 0.75f, 0.20f, 0.20f, 1.00f };
    c[ImGuiCol_ButtonActive]         = { 0.55f, 0.14f, 0.14f, 1.00f };
    c[ImGuiCol_Header]               = { 0.75f, 0.20f, 0.20f, 0.40f };
    c[ImGuiCol_HeaderHovered]        = { 0.75f, 0.20f, 0.20f, 0.65f };
    c[ImGuiCol_HeaderActive]         = { 0.75f, 0.20f, 0.20f, 0.90f };
    c[ImGuiCol_Separator]            = { 0.22f, 0.22f, 0.26f, 1.00f };
    c[ImGuiCol_SeparatorHovered]     = { 0.75f, 0.20f, 0.20f, 0.70f };
    c[ImGuiCol_SeparatorActive]      = { 0.75f, 0.20f, 0.20f, 1.00f };
    c[ImGuiCol_ResizeGrip]           = { 0.75f, 0.20f, 0.20f, 0.20f };
    c[ImGuiCol_ResizeGripHovered]    = { 0.75f, 0.20f, 0.20f, 0.60f };
    c[ImGuiCol_ResizeGripActive]     = { 0.75f, 0.20f, 0.20f, 0.90f };
    c[ImGuiCol_Tab]                  = { 0.15f, 0.15f, 0.18f, 1.00f };
    c[ImGuiCol_TabHovered]           = { 0.75f, 0.20f, 0.20f, 0.80f };
    c[ImGuiCol_TabActive]            = { 0.60f, 0.16f, 0.16f, 1.00f };
    c[ImGuiCol_TabUnfocused]         = { 0.12f, 0.12f, 0.14f, 1.00f };
    c[ImGuiCol_TabUnfocusedActive]   = { 0.30f, 0.14f, 0.14f, 1.00f };
    c[ImGuiCol_TextSelectedBg]       = { 0.75f, 0.20f, 0.20f, 0.35f };
    c[ImGuiCol_NavHighlight]         = { 0.75f, 0.20f, 0.20f, 1.00f };
    c[ImGuiCol_Text]                 = { 0.92f, 0.92f, 0.94f, 1.00f };
    c[ImGuiCol_TextDisabled]         = { 0.45f, 0.45f, 0.50f, 1.00f };
}

static void apply_indigo(ImGuiStyle& st) {
    const float r = 2.f;
    st.WindowBorderSize  = 1.f;
    st.FrameBorderSize   = 1.f;
    st.WindowMinSize     = { 75.f, 50.f };
    st.FramePadding      = {  5.f,  5.f };
    st.ItemSpacing       = {  6.f,  5.f };
    st.ItemInnerSpacing  = {  2.f,  4.f };
    st.WindowRounding    = 0.f;
    st.FrameRounding     = r;
    st.PopupRounding     = 0.f;
    st.PopupBorderSize   = 1.f;
    st.IndentSpacing     = 6.f;
    st.GrabMinSize       = 14.f;
    st.GrabRounding      = r;
    st.ScrollbarSize     = 12.f;
    st.ScrollbarRounding = r;

    ImVec4* c = st.Colors;
    c[ImGuiCol_Text]                 = { 1.00f, 1.00f, 1.00f, 1.00f };
    c[ImGuiCol_TextDisabled]         = { 0.50f, 0.50f, 0.50f, 1.00f };
    c[ImGuiCol_WindowBg]             = { 0.20f, 0.23f, 0.31f, 1.00f };
    c[ImGuiCol_ChildBg]              = { 0.20f, 0.23f, 0.31f, 1.00f };
    c[ImGuiCol_PopupBg]              = { 0.20f, 0.23f, 0.31f, 1.00f };
    c[ImGuiCol_Border]               = { 0.00f, 0.00f, 0.00f, 1.00f };
    c[ImGuiCol_BorderShadow]         = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_FrameBg]              = { 0.25f, 0.28f, 0.38f, 1.00f };
    c[ImGuiCol_FrameBgHovered]       = { 0.25f, 0.28f, 0.38f, 1.00f };
    c[ImGuiCol_FrameBgActive]        = { 0.25f, 0.28f, 0.38f, 1.00f };
    c[ImGuiCol_TitleBg]              = { 0.00f, 0.43f, 1.00f, 1.00f };
    c[ImGuiCol_TitleBgActive]        = { 0.00f, 0.55f, 1.00f, 1.00f };
    c[ImGuiCol_TitleBgCollapsed]     = { 0.10f, 0.69f, 1.00f, 1.00f };
    c[ImGuiCol_MenuBarBg]            = { 0.25f, 0.28f, 0.38f, 1.00f };
    c[ImGuiCol_ScrollbarBg]          = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_ScrollbarGrab]        = { 0.39f, 0.44f, 0.56f, 1.00f };
    c[ImGuiCol_ScrollbarGrabHovered] = { 0.12f, 0.43f, 1.00f, 1.00f };
    c[ImGuiCol_ScrollbarGrabActive]  = { 0.00f, 0.55f, 1.00f, 1.00f };
    c[ImGuiCol_CheckMark]            = { 0.00f, 0.55f, 1.00f, 1.00f };
    c[ImGuiCol_SliderGrab]           = { 0.00f, 0.55f, 1.00f, 1.00f };
    c[ImGuiCol_SliderGrabActive]     = { 0.10f, 0.69f, 1.00f, 1.00f };
    c[ImGuiCol_Button]               = { 0.25f, 0.28f, 0.38f, 1.00f };
    c[ImGuiCol_ButtonHovered]        = { 0.12f, 0.43f, 1.00f, 1.00f };
    c[ImGuiCol_ButtonActive]         = { 0.00f, 0.55f, 1.00f, 1.00f };
    c[ImGuiCol_Header]               = { 0.00f, 0.43f, 1.00f, 1.00f };
    c[ImGuiCol_HeaderHovered]        = { 0.00f, 0.55f, 1.00f, 1.00f };
    c[ImGuiCol_HeaderActive]         = { 0.00f, 0.43f, 1.00f, 1.00f };
    c[ImGuiCol_Separator]            = { 0.43f, 0.43f, 0.50f, 0.50f };
    c[ImGuiCol_SeparatorHovered]     = { 0.10f, 0.40f, 0.75f, 0.78f };
    c[ImGuiCol_SeparatorActive]      = { 0.10f, 0.40f, 0.75f, 1.00f };
    c[ImGuiCol_ResizeGrip]           = { 0.26f, 0.59f, 0.98f, 0.25f };
    c[ImGuiCol_ResizeGripHovered]    = { 0.26f, 0.59f, 0.98f, 0.67f };
    c[ImGuiCol_ResizeGripActive]     = { 0.26f, 0.59f, 0.98f, 0.95f };
    c[ImGuiCol_Tab]                  = { 0.00f, 0.50f, 1.00f, 1.00f };
    c[ImGuiCol_TabHovered]           = { 0.12f, 0.69f, 1.00f, 1.00f };
    c[ImGuiCol_TabActive]            = { 0.12f, 0.69f, 1.00f, 1.00f };
    c[ImGuiCol_TabUnfocused]         = { 0.07f, 0.10f, 0.15f, 0.97f };
    c[ImGuiCol_TabUnfocusedActive]   = { 0.14f, 0.26f, 0.42f, 1.00f };
    c[ImGuiCol_TextSelectedBg]       = { 0.26f, 0.59f, 0.98f, 0.35f };
    c[ImGuiCol_NavHighlight]         = { 0.26f, 0.59f, 0.98f, 1.00f };
}

static void apply_vermillion(ImGuiStyle& st) {
    const float r = 2.f;
    st.WindowBorderSize  = 1.f;
    st.FrameBorderSize   = 1.f;
    st.WindowMinSize     = { 75.f, 50.f };
    st.FramePadding      = {  5.f,  5.f };
    st.ItemSpacing       = {  6.f,  5.f };
    st.ItemInnerSpacing  = {  2.f,  4.f };
    st.WindowRounding    = 0.f;
    st.FrameRounding     = r;
    st.PopupRounding     = 0.f;
    st.GrabMinSize       = 14.f;
    st.GrabRounding      = r;
    st.ScrollbarSize     = 12.f;
    st.ScrollbarRounding = r;

    ImVec4* c = st.Colors;
    c[ImGuiCol_Text]                 = { 1.00f, 1.00f, 1.00f, 0.75f };
    c[ImGuiCol_TextDisabled]         = { 1.00f, 0.18f, 0.29f, 0.78f };
    c[ImGuiCol_WindowBg]             = { 0.17f, 0.20f, 0.25f, 1.00f };
    c[ImGuiCol_ChildBg]              = { 0.20f, 0.22f, 0.27f, 0.57f };
    c[ImGuiCol_PopupBg]              = { 0.17f, 0.20f, 0.25f, 1.00f };
    c[ImGuiCol_Border]               = { 0.00f, 0.00f, 0.00f, 1.00f };
    c[ImGuiCol_BorderShadow]         = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_FrameBg]              = { 0.22f, 0.25f, 0.31f, 1.00f };
    c[ImGuiCol_FrameBgHovered]       = { 0.22f, 0.25f, 0.31f, 1.00f };
    c[ImGuiCol_FrameBgActive]        = { 0.22f, 0.25f, 0.31f, 1.00f };
    c[ImGuiCol_TitleBg]              = { 0.65f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_TitleBgActive]        = { 0.78f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_TitleBgCollapsed]     = { 0.78f, 0.18f, 0.29f, 0.60f };
    c[ImGuiCol_ScrollbarBg]          = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_ScrollbarGrab]        = { 0.65f, 0.18f, 0.29f, 0.37f };
    c[ImGuiCol_ScrollbarGrabHovered] = { 0.78f, 0.18f, 0.29f, 0.78f };
    c[ImGuiCol_ScrollbarGrabActive]  = { 0.78f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_CheckMark]            = { 0.71f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_SliderGrab]           = { 0.78f, 0.18f, 0.29f, 0.37f };
    c[ImGuiCol_SliderGrabActive]     = { 0.92f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_Button]               = { 0.65f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_ButtonHovered]        = { 0.78f, 0.18f, 0.29f, 0.86f };
    c[ImGuiCol_ButtonActive]         = { 0.78f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_Header]               = { 0.78f, 0.18f, 0.29f, 0.76f };
    c[ImGuiCol_HeaderHovered]        = { 0.78f, 0.18f, 0.29f, 0.86f };
    c[ImGuiCol_HeaderActive]         = { 0.78f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_Separator]            = { 0.15f, 0.00f, 0.00f, 0.35f };
    c[ImGuiCol_SeparatorHovered]     = { 0.78f, 0.18f, 0.29f, 0.59f };
    c[ImGuiCol_SeparatorActive]      = { 0.78f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_ResizeGrip]           = { 0.78f, 0.18f, 0.29f, 0.63f };
    c[ImGuiCol_ResizeGripHovered]    = { 0.78f, 0.18f, 0.29f, 0.78f };
    c[ImGuiCol_ResizeGripActive]     = { 0.78f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_Tab]                  = { 0.78f, 0.18f, 0.29f, 0.76f };
    c[ImGuiCol_TabHovered]           = { 0.78f, 0.18f, 0.29f, 0.86f };
    c[ImGuiCol_TabActive]            = { 0.78f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_TabUnfocused]         = { 0.07f, 0.10f, 0.15f, 0.97f };
    c[ImGuiCol_TabUnfocusedActive]   = { 0.14f, 0.26f, 0.42f, 1.00f };
    c[ImGuiCol_TextSelectedBg]       = { 0.92f, 0.18f, 0.29f, 0.43f };
    c[ImGuiCol_NavHighlight]         = { 0.45f, 0.45f, 0.90f, 0.80f };
}

static void apply_classic_steam(ImGuiStyle& st) {
    st.WindowRounding    = 0.f;
    st.ChildRounding     = 0.f;
    st.FrameRounding     = 0.f;
    st.PopupRounding     = 0.f;
    st.ScrollbarRounding = 0.f;
    st.GrabRounding      = 0.f;
    st.TabRounding       = 0.f;
    st.WindowBorderSize  = 1.f;
    st.ChildBorderSize   = 1.f;
    st.PopupBorderSize   = 1.f;
    st.FrameBorderSize   = 1.f;
    st.WindowPadding     = { 8.f, 8.f };
    st.FramePadding      = { 4.f, 3.f };
    st.ItemSpacing       = { 8.f, 4.f };
    st.ItemInnerSpacing  = { 4.f, 4.f };
    st.IndentSpacing     = 21.f;
    st.ScrollbarSize     = 14.f;
    st.GrabMinSize       = 10.f;

    ImVec4* c = st.Colors;
    c[ImGuiCol_Text]                 = { 1.000f, 1.000f, 1.000f, 1.00f };
    c[ImGuiCol_TextDisabled]         = { 0.498f, 0.498f, 0.498f, 1.00f };
    c[ImGuiCol_WindowBg]             = { 0.286f, 0.337f, 0.259f, 1.00f };
    c[ImGuiCol_ChildBg]              = { 0.286f, 0.337f, 0.259f, 1.00f };
    c[ImGuiCol_PopupBg]              = { 0.239f, 0.267f, 0.200f, 1.00f };
    c[ImGuiCol_Border]               = { 0.537f, 0.569f, 0.510f, 0.50f };
    c[ImGuiCol_BorderShadow]         = { 0.137f, 0.157f, 0.110f, 0.52f };
    c[ImGuiCol_FrameBg]              = { 0.239f, 0.267f, 0.200f, 1.00f };
    c[ImGuiCol_FrameBgHovered]       = { 0.267f, 0.298f, 0.227f, 1.00f };
    c[ImGuiCol_FrameBgActive]        = { 0.298f, 0.337f, 0.259f, 1.00f };
    c[ImGuiCol_TitleBg]              = { 0.239f, 0.267f, 0.200f, 1.00f };
    c[ImGuiCol_TitleBgActive]        = { 0.286f, 0.337f, 0.259f, 1.00f };
    c[ImGuiCol_TitleBgCollapsed]     = { 0.000f, 0.000f, 0.000f, 0.51f };
    c[ImGuiCol_ScrollbarBg]          = { 0.349f, 0.420f, 0.310f, 1.00f };
    c[ImGuiCol_ScrollbarGrab]        = { 0.278f, 0.318f, 0.239f, 1.00f };
    c[ImGuiCol_ScrollbarGrabHovered] = { 0.247f, 0.298f, 0.220f, 1.00f };
    c[ImGuiCol_ScrollbarGrabActive]  = { 0.227f, 0.267f, 0.208f, 1.00f };
    c[ImGuiCol_CheckMark]            = { 0.588f, 0.537f, 0.176f, 1.00f };
    c[ImGuiCol_SliderGrab]           = { 0.349f, 0.420f, 0.310f, 1.00f };
    c[ImGuiCol_SliderGrabActive]     = { 0.537f, 0.569f, 0.510f, 0.50f };
    c[ImGuiCol_Button]               = { 0.286f, 0.337f, 0.259f, 0.40f };
    c[ImGuiCol_ButtonHovered]        = { 0.349f, 0.420f, 0.310f, 1.00f };
    c[ImGuiCol_ButtonActive]         = { 0.537f, 0.569f, 0.510f, 0.50f };
    c[ImGuiCol_Header]               = { 0.349f, 0.420f, 0.310f, 1.00f };
    c[ImGuiCol_HeaderHovered]        = { 0.349f, 0.420f, 0.310f, 0.60f };
    c[ImGuiCol_HeaderActive]         = { 0.537f, 0.569f, 0.510f, 0.50f };
    c[ImGuiCol_Separator]            = { 0.137f, 0.157f, 0.110f, 1.00f };
    c[ImGuiCol_SeparatorHovered]     = { 0.537f, 0.569f, 0.510f, 1.00f };
    c[ImGuiCol_SeparatorActive]      = { 0.588f, 0.537f, 0.176f, 1.00f };
    c[ImGuiCol_Tab]                  = { 0.349f, 0.420f, 0.310f, 1.00f };
    c[ImGuiCol_TabHovered]           = { 0.537f, 0.569f, 0.510f, 0.78f };
    c[ImGuiCol_TabActive]            = { 0.588f, 0.537f, 0.176f, 1.00f };
    c[ImGuiCol_TabUnfocused]         = { 0.239f, 0.267f, 0.200f, 1.00f };
    c[ImGuiCol_TabUnfocusedActive]   = { 0.349f, 0.420f, 0.310f, 1.00f };
    c[ImGuiCol_TextSelectedBg]       = { 0.588f, 0.537f, 0.176f, 1.00f };
    c[ImGuiCol_NavHighlight]         = { 0.588f, 0.537f, 0.176f, 1.00f };
}



static void load_bg_crimson() {
    if (g_bg_crimson_tex != (ImTextureID)0) return;
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        BG_WIDTH, BG_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, BG_RGBA);
    glBindTexture(GL_TEXTURE_2D, 0);
    g_bg_crimson_tex = (ImTextureID)(uintptr_t)tex;
}

static void apply_crimson(ImGuiStyle& st) {
    st.WindowRounding    = 4.f;
    st.ChildRounding     = 3.f;
    st.FrameRounding     = 3.f;
    st.PopupRounding     = 4.f;
    st.ScrollbarRounding = 3.f;
    st.GrabRounding      = 3.f;
    st.TabRounding       = 3.f;
    st.WindowBorderSize  = 1.f;
    st.FrameBorderSize   = 0.f;
    st.WindowPadding     = { 14.f, 14.f };
    st.FramePadding      = { 10.f,  6.f };
    st.ItemSpacing       = {  8.f,  5.f };
    st.ItemInnerSpacing  = {  6.f,  3.f };
    st.ScrollbarSize     = 10.f;
    st.GrabMinSize       = 8.f;
    st.IndentSpacing     = 18.f;

    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg]             = { 0.06f, 0.04f, 0.04f, 0.00f };
    c[ImGuiCol_ChildBg]              = { 0.08f, 0.05f, 0.05f, 1.00f };
    c[ImGuiCol_PopupBg]              = { 0.07f, 0.04f, 0.04f, 1.00f };
    c[ImGuiCol_Border]               = { 0.25f, 0.08f, 0.08f, 1.00f };
    c[ImGuiCol_BorderShadow]         = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_FrameBg]              = { 0.12f, 0.07f, 0.07f, 1.00f };
    c[ImGuiCol_FrameBgHovered]       = { 0.18f, 0.09f, 0.09f, 1.00f };
    c[ImGuiCol_FrameBgActive]        = { 0.22f, 0.10f, 0.10f, 1.00f };
    c[ImGuiCol_TitleBg]              = { 0.08f, 0.04f, 0.04f, 1.00f };
    c[ImGuiCol_TitleBgActive]        = { 0.10f, 0.05f, 0.05f, 1.00f };
    c[ImGuiCol_TitleBgCollapsed]     = { 0.06f, 0.03f, 0.03f, 1.00f };
    c[ImGuiCol_ScrollbarBg]          = { 0.06f, 0.04f, 0.04f, 1.00f };
    c[ImGuiCol_ScrollbarGrab]        = { 0.30f, 0.08f, 0.08f, 1.00f };
    c[ImGuiCol_ScrollbarGrabHovered] = { 0.45f, 0.10f, 0.10f, 1.00f };
    c[ImGuiCol_ScrollbarGrabActive]  = { 0.60f, 0.12f, 0.12f, 1.00f };
    c[ImGuiCol_CheckMark]            = { 0.85f, 0.15f, 0.15f, 1.00f };
    c[ImGuiCol_SliderGrab]           = { 0.70f, 0.12f, 0.12f, 1.00f };
    c[ImGuiCol_SliderGrabActive]     = { 0.90f, 0.18f, 0.18f, 1.00f };
    c[ImGuiCol_Button]               = { 0.18f, 0.07f, 0.07f, 1.00f };
    c[ImGuiCol_ButtonHovered]        = { 0.55f, 0.10f, 0.10f, 1.00f };
    c[ImGuiCol_ButtonActive]         = { 0.72f, 0.14f, 0.14f, 1.00f };
    c[ImGuiCol_Header]               = { 0.45f, 0.08f, 0.08f, 0.55f };
    c[ImGuiCol_HeaderHovered]        = { 0.55f, 0.10f, 0.10f, 0.75f };
    c[ImGuiCol_HeaderActive]         = { 0.70f, 0.14f, 0.14f, 1.00f };
    c[ImGuiCol_Separator]            = { 0.22f, 0.07f, 0.07f, 1.00f };
    c[ImGuiCol_SeparatorHovered]     = { 0.55f, 0.10f, 0.10f, 0.80f };
    c[ImGuiCol_SeparatorActive]      = { 0.72f, 0.14f, 0.14f, 1.00f };
    c[ImGuiCol_ResizeGrip]           = { 0.45f, 0.08f, 0.08f, 0.25f };
    c[ImGuiCol_ResizeGripHovered]    = { 0.60f, 0.10f, 0.10f, 0.65f };
    c[ImGuiCol_ResizeGripActive]     = { 0.75f, 0.14f, 0.14f, 0.95f };
    c[ImGuiCol_Tab]                  = { 0.14f, 0.06f, 0.06f, 1.00f };
    c[ImGuiCol_TabHovered]           = { 0.55f, 0.10f, 0.10f, 0.85f };
    c[ImGuiCol_TabActive]            = { 0.45f, 0.08f, 0.08f, 1.00f };
    c[ImGuiCol_TabUnfocused]         = { 0.08f, 0.04f, 0.04f, 1.00f };
    c[ImGuiCol_TabUnfocusedActive]   = { 0.18f, 0.07f, 0.07f, 1.00f };
    c[ImGuiCol_TextSelectedBg]       = { 0.60f, 0.10f, 0.10f, 0.40f };
    c[ImGuiCol_NavHighlight]         = { 0.72f, 0.14f, 0.14f, 1.00f };
    c[ImGuiCol_Text]                 = { 0.92f, 0.88f, 0.88f, 1.00f };
    c[ImGuiCol_TextDisabled]         = { 0.40f, 0.28f, 0.28f, 1.00f };
}

void ui_apply_theme(AppTheme t) {
    g_current_theme = t;
    g_theme         = static_cast<int>(t);
    save_settings();
    ImGuiStyle& st  = ImGui::GetStyle();

    switch (t) {
        case AppTheme::Indigo:       apply_indigo(st);        break;
        case AppTheme::Vermillion:   apply_vermillion(st);    break;
        case AppTheme::ClassicSteam: apply_classic_steam(st); break;
        case AppTheme::Crimson:      apply_crimson(st); load_bg_crimson(); break;
        default:                     apply_dark(st);          break;
    }
}

static void render_settings_panel(ImVec2 ds) {
    ImGui::SetNextWindowPos({ ds.x - 210.f, 34.f });
    ImGui::SetNextWindowSize({ 200.f, 0.f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    { 12.f, 10.f });

    ImGui::Begin("##settings_panel", &g_settings_open,
        ImGuiWindowFlags_NoDecoration  |
        ImGuiWindowFlags_NoResize      |
        ImGuiWindowFlags_NoMove        |
        ImGuiWindowFlags_NoScrollbar   |
        ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextDisabled("Тема оформления");
    ImGui::Spacing();

    struct ThemeEntry { const char* name; AppTheme id; };
    ThemeEntry themes[] = {
        { "Dark (default)", AppTheme::Dark         },
        { "Indigo",         AppTheme::Indigo       },
        { "Vermillion",     AppTheme::Vermillion   },
        { "Classic Steam",  AppTheme::ClassicSteam },
        { "Artem(only)",    AppTheme::Crimson      },
    };

    for (auto& e : themes) {
        bool sel = (g_current_theme == e.id);
        ImGui::PushStyleColor(ImGuiCol_Text,
            sel ? ImVec4{ 0.92f, 0.92f, 0.94f, 1.f }
                : ImVec4{ 0.55f, 0.55f, 0.60f, 1.f });

        if (ImGui::Selectable(e.name, sel)) {
            ui_apply_theme(e.id);
            g_settings_open = false;
        }

        ImGui::PopStyleColor();
    }

    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
        ImGui::IsMouseClicked(0)) {
        g_settings_open = false;
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
}

static void render_palette_panel(ImVec2 ds) {
    ImGui::SetNextWindowPos({ ds.x - 250.f, 34.f });
    ImGui::SetNextWindowSize({ 238.f, 0.f });
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg,  { 0.05f, 0.05f, 0.05f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_Border,    { 0.30f, 0.08f, 0.08f, 1.00f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    { 12.f, 10.f });

    ImGui::Begin("##palette_panel", &g_palette_open,
        ImGuiWindowFlags_NoDecoration  |
        ImGuiWindowFlags_NoResize      |
        ImGuiWindowFlags_NoMove        |
        ImGuiWindowFlags_NoScrollbar   |
        ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextDisabled("Цвета списков (Crimson)");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Фон строки");
    ImGui::ColorEdit4("##child_bg",    (float*)&g_crimson_child_bg,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

    ImGui::Spacing();
    ImGui::Text("Выделение");
    ImGui::ColorEdit4("##selected_bg", (float*)&g_crimson_selected_bg,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

    ImGui::Spacing();
    ImGui::Text("Hover");
    ImGui::ColorEdit4("##hovered_bg",  (float*)&g_crimson_hovered_bg,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

    ImGui::Spacing();
    ImGui::Text("Текст");
    ImGui::ColorEdit4("##text_col",    (float*)&g_crimson_text,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Сбросить", { -1, 0 })) {
        g_crimson_child_bg    = { 0.12f, 0.05f, 0.05f, 1.00f };
        g_crimson_selected_bg = { 0.55f, 0.08f, 0.08f, 1.00f };
        g_crimson_hovered_bg  = { 0.30f, 0.06f, 0.06f, 1.00f };
        g_crimson_text        = { 0.95f, 0.88f, 0.88f, 1.00f };
    }

    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
                                ImGuiHoveredFlags_ChildWindows) &&
        ImGui::IsMouseClicked(0)) {
        g_palette_open = false;
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

void ui_render_main(ImFont* font_big, ImVec2 ds) {
    avatar_flush_pending();

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(ds);
    ImGui::Begin("Main", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

    if (g_current_theme == AppTheme::Crimson && g_bg_crimson_tex != (ImTextureID)0) {
        ImVec2 wpos = ImGui::GetWindowPos();
        ImGui::GetWindowDrawList()->AddImage(
            g_bg_crimson_tex,
            wpos,
            { wpos.x + ds.x, wpos.y + ds.y },
            { 0, 0 }, { 1, 1 },
            IM_COL32(255, 255, 255, 255));
    }

    if (font_big) ImGui::PushFont(font_big);
    {
        const char* title = "DOTA 2 CFG";
        ImDrawList* dl    = ImGui::GetWindowDrawList();
        ImVec2      pos   = ImGui::GetCursorScreenPos();
        float       th    = ImGui::CalcTextSize(title).y;

        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
            pos, IM_COL32(220, 40, 40, 255), title);

        float tw = ImGui::CalcTextSize(title).x;
        dl->AddRectFilledMultiColor(
            { pos.x, pos.y + th + 2.f },
            { pos.x + tw, pos.y + th + 4.f },
            IM_COL32(220, 40, 40, 220),
            IM_COL32(180, 20, 20, 0),
            IM_COL32(180, 20, 20, 0),
            IM_COL32(220, 40, 40, 220));

        ImGui::Dummy({ tw, th + 4.f });
    }
    if (font_big) ImGui::PopFont();

    ImGui::SameLine(0, 8.f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.f);
    ImGui::TextDisabled("by OutTuna");
    if (g_current_theme == AppTheme::Crimson) {
        ImGui::SameLine(ds.x - 70.f);
        ImGui::PushStyleColor(ImGuiCol_Button,        { 0.00f, 0.00f, 0.00f, 0.00f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.40f, 0.08f, 0.08f, 1.00f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  { 0.25f, 0.05f, 0.05f, 1.00f });
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);
        if (ImGui::Button("##palette", { 26, 22 }))
            g_palette_open = !g_palette_open;
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImDrawList* pdl   = ImGui::GetWindowDrawList();
        ImVec2      pmin  = ImGui::GetItemRectMin();
        ImVec2      pmax  = ImGui::GetItemRectMax();
        float       pcx   = (pmin.x + pmax.x) * 0.5f;
        float       pcy   = (pmin.y + pmax.y) * 0.5f;
        float       pr    = 4.5f;
        ImVec2 dots[4] = {
            { pcx - pr, pcy - pr }, { pcx + pr, pcy - pr },
            { pcx - pr, pcy + pr }, { pcx + pr, pcy + pr }
        };
        ImU32 dot_cols[4] = {
            IM_COL32(220, 60, 60, 220),  IM_COL32(60, 120, 220, 220),
            IM_COL32(60, 200, 80, 220),  IM_COL32(220, 180, 40, 220)
        };
        for (int i = 0; i < 4; i++)
            pdl->AddCircleFilled(dots[i], 3.f, dot_cols[i]);
    }

    ImGui::SameLine(ds.x - 38.f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);

    ImGui::PushStyleColor(ImGuiCol_Button,        { 0.00f, 0.00f, 0.00f, 0.00f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  { 0.25f, 0.25f, 0.28f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,   { 0.18f, 0.18f, 0.20f, 1.00f });
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);

    if (ImGui::Button("##gear", { 26, 22 }))
        g_settings_open = !g_settings_open;

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImDrawList* dl  = ImGui::GetWindowDrawList();
    ImVec2 bmin     = ImGui::GetItemRectMin();
    ImVec2 bmax     = ImGui::GetItemRectMax();
    ImVec2 ctr      = { (bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f };
    float  t        = (float)glfwGetTime();
    float  ang      = g_settings_open ? t * 1.2f : 0.f;
    ImU32  col      = IM_COL32(160, 160, 170, 220);
    const float PI  = 3.14159f;

    for (int i = 0; i < 8; i++) {
        float a0 = ang + i * (PI * 2.f / 8.f);
        float a1 = a0 + 0.35f;
        dl->PathArcTo(ctr, 6.5f, a0, a1, 4);
        dl->PathArcTo(ctr, 3.5f, a1, a0 + PI * 2.f / 8.f, 4);
        dl->PathFillConvex(col);
    }
    dl->AddCircleFilled(ctr, 2.8f, col);

    ImGui::Separator();

    static bool open_src = false;
    static bool open_dst = false;

    ImGui::TextDisabled("Откуда конфиг");
    ImGui::PushItemWidth(-1);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, { 0.28f, 0.28f, 0.32f, 1.f });
    ImGui::InputText("##src_path", src_path, 256, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();
    if (ImGui::IsItemClicked()) open_src = true;
    ImGui::PopItemWidth();

    ImGui::Spacing();

    ImGui::TextDisabled("Куда конфиг");
    ImGui::PushItemWidth(-1);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, { 0.28f, 0.28f, 0.32f, 1.f });
    ImGui::InputText("##dst_path", dst_path, 256, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();
    if (ImGui::IsItemClicked()) open_dst = true;
    ImGui::PopItemWidth();

    ImGui::Spacing();

#ifdef _WIN32
    if (open_src) {
        open_src = false;
        auto p = browse_for_folder("Откуда конфиг");
        if (!p.empty()) { strncpy(src_path, p.c_str(), 255); src_path[255] = 0; }
    }
    if (open_dst) {
        open_dst = false;
        auto p = browse_for_folder("Куда конфиг");
        if (!p.empty()) { strncpy(dst_path, p.c_str(), 255); dst_path[255] = 0; }
    }
#endif

    if (ImGui::Button("SCAN FOLDERS", { -1, 32 }))
        std::thread(scan_thread).detach();

    const float AVATAR_SIZE  = 24.f;
    const float ROW_H        = AVATAR_SIZE + 6.f;
    const float bottom_h     = 38.f + ImGui::GetTextLineHeightWithSpacing()
                               + ImGui::GetStyle().ItemSpacing.y * 3
                               + ImGui::GetStyle().WindowPadding.y;
    const float list_h       = ImGui::GetContentRegionAvail().y - bottom_h;

    ImGui::Columns(2, "lists", true);
    ImGui::Text("From (Config)");

    auto render_account_list = [&](
        const std::vector<std::string>& list,
        int& selected,
        const char* child_id)
    {
        bool crimson = (g_current_theme == AppTheme::Crimson);

        ImGui::BeginChild(child_id, { 0, list_h }, true);

        for (int i = 0; i < (int)list.size(); i++) {
            const std::string& id  = list[i];
            std::string nick       = nick_cache.count(id) ? nick_cache[id] : id;
            std::string label      = nick + "  (" + id + ")";
            bool        sel        = (i == selected);

            ImGui::PushID(i);

            ImVec2 row_pos = ImGui::GetCursorScreenPos();

            if (crimson) {
                ImVec4 row_col = sel ? g_crimson_selected_bg : g_crimson_child_bg;
                ImGui::GetWindowDrawList()->AddRectFilled(
                    row_pos,
                    { row_pos.x + ImGui::GetContentRegionAvail().x, row_pos.y + ROW_H },
                    ImGui::ColorConvertFloat4ToU32(row_col));
            }

            if (crimson) {
                ImGui::PushStyleColor(ImGuiCol_Header,        g_crimson_selected_bg);
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, g_crimson_hovered_bg);
                ImGui::PushStyleColor(ImGuiCol_HeaderActive,  g_crimson_selected_bg);
            }

            if (ImGui::Selectable("##row", sel,
                    ImGuiSelectableFlags_None, { 0, ROW_H })) {
                selected = i;
            }

            if (crimson) ImGui::PopStyleColor(3);

            ImDrawList* dl      = ImGui::GetWindowDrawList();
            const float pad     = 5.f;
            const float radius  = AVATAR_SIZE * 0.5f;
            ImVec2      av_ctr  = {
                row_pos.x + pad + radius,
                row_pos.y + ROW_H * 0.5f
            };

            ImTextureID tex = avatar_get(id);
            if (tex != (ImTextureID)0) {
                dl->AddImage(
                    tex,
                    { av_ctr.x - radius, av_ctr.y - radius },
                    { av_ctr.x + radius, av_ctr.y + radius });
            } else {
                dl->AddRectFilled(
                    { av_ctr.x - radius, av_ctr.y - radius },
                    { av_ctr.x + radius, av_ctr.y + radius },
                    IM_COL32(50, 50, 55, 220), 3.f);
                dl->AddRect(
                    { av_ctr.x - radius, av_ctr.y - radius },
                    { av_ctr.x + radius, av_ctr.y + radius },
                    IM_COL32(80, 80, 90, 180), 3.f);
            }

            float text_x = row_pos.x + pad + AVATAR_SIZE + 8.f;
            float text_y = row_pos.y + (ROW_H - ImGui::GetTextLineHeight()) * 0.5f;

            ImU32 text_col;
            if (crimson) {
                ImVec4 tc = g_crimson_text;
                text_col = IM_COL32(
                    (int)(tc.x*255), (int)(tc.y*255),
                    (int)(tc.z*255), (int)(tc.w*255));
            } else {
                text_col = sel ? IM_COL32(255,255,255,255)
                               : IM_COL32(180,180,185,255);
            }

            dl->AddText({ text_x, text_y }, text_col, label.c_str());

            ImGui::PopID();
        }

        ImGui::EndChild();
    };

    render_account_list(src_list, selected_src, "##src_list");

    ImGui::NextColumn();
    ImGui::Text("To (Account)");
    render_account_list(dst_list, selected_dst, "##dst_list");
    ImGui::Columns(1);

    ImGui::Separator();
    if (ImGui::Button("COPY CONFIG NOW", { -1, 38 })) copy_config();
    ImGui::Text("Status: %s", status_msg.c_str());

    ImGui::End();

    if (g_settings_open)
        render_settings_panel(ds);

    if (g_palette_open && g_current_theme == AppTheme::Crimson)
        render_palette_panel(ds);
}



void ui_render_success_popup(ImFont* font_big, ImVec2 ds, float delta_time) {
    if (!g_success.show) return;

    const float OPEN_DUR  = 0.30f;
    const float CLOSE_DUR = 0.25f;

    if (!g_success.closing) {
        g_success.anim_t += delta_time;
    } else {
        g_success.close_t += delta_time;
        if (g_success.close_t >= CLOSE_DUR) {
            g_success.show = false;
            return;
        }
    }

    float t_open    = (g_success.anim_t / OPEN_DUR < 1.f) ? g_success.anim_t / OPEN_DUR : 1.f;
    float ease_open = t_open * t_open * (3.f - 2.f * t_open);

    float t_close   = g_success.closing ? (g_success.close_t / CLOSE_DUR) : 0.f;
    if (t_close > 1.f) t_close = 1.f;
    float ease_close = t_close * t_close;

    float scale = (0.82f + 0.18f * ease_open) * (1.f - 0.15f * ease_close);
    float alpha = ease_open * (1.f - ease_close);
    float fly_y = (1.f - ease_open) * 30.f + ease_close * (-20.f);

    const float POP_W    = 460.f;
    const float POP_H    = 300.f;
    const float ROUNDING = 14.f;
    const float cx       = ds.x * 0.5f;
    const float cy       = ds.y * 0.5f + fly_y;
    const float sw       = POP_W * scale;
    const float sh       = POP_H * scale;
    const float pop_x    = cx - sw * 0.5f;
    const float pop_y    = cy - sh * 0.5f;
    const float pop_x2   = cx + sw * 0.5f;
    const float pop_y2   = cy + sh * 0.5f;
    const float time     = (float)glfwGetTime();
    const float pulse    = 0.5f + 0.5f * sinf(time * 2.8f);


    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(ds);
    ImGui::SetNextWindowBgAlpha(0.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
    ImGui::Begin("##dim", NULL,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoResize     |
        ImGuiWindowFlags_NoInputs     |
        ImGuiWindowFlags_NoNav);

    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled({ 0, 0 }, ds, IM_COL32(0, 0, 0, (int)(150 * alpha)));

    struct GlowLayer { float p; int a; };
    GlowLayer glows[] = { { 10, 60 }, { 22, 40 }, { 38, 24 }, { 58, 12 }, { 82, 6 } };
    for (auto& g : glows) {
        dl->AddRectFilled(
            { pop_x - g.p, pop_y - g.p },
            { pop_x2 + g.p, pop_y2 + g.p },
            IM_COL32(30, 200, 80, (int)(g.a * alpha)),
            ROUNDING + g.p * 0.55f);
    }

    dl->AddRectFilled({ pop_x, pop_y }, { pop_x2, pop_y2 },
        IM_COL32(16, 17, 20, (int)(255 * alpha)), ROUNDING);

    float bar_pulse = 0.75f + 0.25f * sinf(time * 2.f);
    dl->AddRectFilled({ pop_x, pop_y }, { pop_x2, pop_y + 3.5f },
        IM_COL32(35, 185, 80, (int)(255 * bar_pulse * alpha)),
        ROUNDING, ImDrawFlags_RoundCornersTop);

    dl->AddRect({ pop_x, pop_y }, { pop_x2, pop_y2 },
        IM_COL32(35, 185, 80, (int)((100 + 60 * pulse) * alpha)),
        ROUNDING, 0, 1.f);

    ImGui::End();
    ImGui::PopStyleColor();


    ImGui::SetNextWindowPos({ pop_x, pop_y });
    ImGui::SetNextWindowSize({ sw, sh });
    ImGui::SetNextWindowBgAlpha(0.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_Border,   { 0, 0, 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   ROUNDING);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    { 20.f * scale, 14.f * scale });

    ImGui::Begin("##success_popup", NULL,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoResize     |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

    if (font_big) ImGui::PushFont(font_big);
    {
        const char* title = "SUCCESS";
        float tw = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((sw - tw) * 0.5f);
        float gv = 0.72f + 0.28f * sinf(time * 2.2f);
        ImGui::TextColored({ 0.10f, gv, 0.35f, 1.f }, "%s", title);
    }
    if (font_big) ImGui::PopFont();

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, { 0.15f, 0.70f, 0.30f, 0.25f });
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    auto row = [](const char* lbl, ImVec4 lc, const char* val, ImVec4 vc) {
        ImGui::TextColored(lc, "%s", lbl);
        ImGui::SameLine();
        ImGui::TextColored(vc, "%s", val);
    };

    std::string src_d = g_success.src_nick + "  (ID: " + g_success.src_id + ")";
    std::string dst_d = g_success.dst_nick + "  (ID: " + g_success.dst_id + ")";

    row("Откуда: ", { 0.55f, 0.55f, 0.60f, 1 }, src_d.c_str(), { 0.92f, 0.92f, 0.94f, 1 });
    row("Куда:   ", { 0.55f, 0.55f, 0.60f, 1 }, dst_d.c_str(), { 0.92f, 0.92f, 0.94f, 1 });

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, { 0.55f, 0.55f, 0.60f, 0.20f });
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::TextColored({ 0.55f, 0.55f, 0.60f, 1 }, "Source:");
    ImGui::SameLine();
    ImGui::TextColored({ 0.55f, 0.55f, 0.60f, 1 }, "%s", g_success.src_folder.c_str());

    ImGui::TextColored({ 0.55f, 0.55f, 0.60f, 1 }, "Dest:  ");
    ImGui::SameLine();
    ImGui::TextColored({ 0.55f, 0.55f, 0.60f, 1 }, "%s", g_success.dst_folder.c_str());

    ImGui::Spacing();
    ImGui::Spacing();

    const float btn_w  = 120.f * scale;
    const float btn_h  = 32.f  * scale;
    const float btn_gv = 0.55f + 0.12f * pulse;

    ImGui::SetCursorPosX((sw - btn_w) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Button,        { 0.10f, btn_gv, 0.20f, 1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.14f, 0.78f,  0.28f, 1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  { 0.07f, 0.42f,  0.15f, 1.f });
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);

    if (ImGui::Button("OK", { btn_w, btn_h })) {
        g_success.closing = true;
        g_success.close_t = 0.f;
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::PopStyleVar();
    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}