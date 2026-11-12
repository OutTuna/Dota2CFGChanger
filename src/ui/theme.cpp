#include "ui_internal.h"
#include "app.h"
#include "embedded_themes.h"
#include "stb_image.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <unordered_map>
#include <nlohmann/json.hpp>

AppTheme g_current_theme = AppTheme::Dark;

ImTextureID g_bg_crimson_tex = (ImTextureID)0;

ImVec4 g_crimson_child_bg = { 0.12f, 0.05f, 0.05f, 1.00f };
ImVec4 g_crimson_selected_bg = { 0.55f, 0.08f, 0.08f, 1.00f };
ImVec4 g_crimson_hovered_bg = { 0.30f, 0.06f, 0.06f, 1.00f };
ImVec4 g_crimson_text = { 0.95f, 0.88f, 0.88f, 1.00f };

std::string themes_dir() {
    auto folder = std::filesystem::u8path(config_dir()) / "themes";
    std::error_code error;
    if (!std::filesystem::is_directory(folder, error)) folder = std::filesystem::u8path(exe_dir()) / "themes";
    return folder.u8string() + "/";
}

ThemeData g_theme_data[5] = {
    { "Dark (default)", 0, "Default dark theme", false, "" },
    { "Indigo", 1, "Blue indigo theme with borders", false, "" },
    { "Vermillion", 2, "Red vermillion theme", false, "" },
    { "Classic Steam", 3, "Classic Steam green theme", false, "" },
    { "Artem(Only)", 4, "Crimson theme with custom background and palette editor", true, "bg_crimson.png" },
};

static ImVec4 json_to_vec4(const nlohmann::json& arr, ImVec4 fallback) {
    if (!arr.is_array() || arr.size() < 4) return fallback;
    return ImVec4(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>(), arr[3].get<float>());
}

static ImVec2 json_to_vec2(const nlohmann::json& arr, ImVec2 fallback) {
    if (!arr.is_array() || arr.size() < 2) return fallback;
    return ImVec2(arr[0].get<float>(), arr[1].get<float>());
}

static const char* theme_filename(AppTheme t) {
    switch (t) {
    case AppTheme::Dark: return "theme_dark.json";
    case AppTheme::Indigo: return "theme_indigo.json";
    case AppTheme::Vermillion: return "theme_vermillion.json";
    case AppTheme::ClassicSteam: return "theme_classic_steam.json";
    case AppTheme::Crimson: return "theme_crimson.json";
    default: return "theme_dark.json";
    }
}

static const std::unordered_map<std::string, ImGuiCol_>& color_name_map() {
    static const std::unordered_map<std::string, ImGuiCol_> m = {
        {"WindowBg", ImGuiCol_WindowBg}, {"ChildBg", ImGuiCol_ChildBg},
        {"PopupBg", ImGuiCol_PopupBg}, {"Border", ImGuiCol_Border},
        {"BorderShadow", ImGuiCol_BorderShadow}, {"FrameBg", ImGuiCol_FrameBg},
        {"FrameBgHovered", ImGuiCol_FrameBgHovered}, {"FrameBgActive", ImGuiCol_FrameBgActive},
        {"TitleBg", ImGuiCol_TitleBg}, {"TitleBgActive", ImGuiCol_TitleBgActive},
        {"TitleBgCollapsed", ImGuiCol_TitleBgCollapsed}, {"MenuBarBg", ImGuiCol_MenuBarBg},
        {"ScrollbarBg", ImGuiCol_ScrollbarBg}, {"ScrollbarGrab", ImGuiCol_ScrollbarGrab},
        {"ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered}, {"ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive},
        {"CheckMark", ImGuiCol_CheckMark}, {"SliderGrab", ImGuiCol_SliderGrab},
        {"SliderGrabActive", ImGuiCol_SliderGrabActive}, {"Button", ImGuiCol_Button},
        {"ButtonHovered", ImGuiCol_ButtonHovered}, {"ButtonActive", ImGuiCol_ButtonActive},
        {"Header", ImGuiCol_Header}, {"HeaderHovered", ImGuiCol_HeaderHovered},
        {"HeaderActive", ImGuiCol_HeaderActive}, {"Separator", ImGuiCol_Separator},
        {"SeparatorHovered", ImGuiCol_SeparatorHovered}, {"SeparatorActive", ImGuiCol_SeparatorActive},
        {"ResizeGrip", ImGuiCol_ResizeGrip}, {"ResizeGripHovered", ImGuiCol_ResizeGripHovered},
        {"ResizeGripActive", ImGuiCol_ResizeGripActive}, {"Tab", ImGuiCol_Tab},
        {"TabHovered", ImGuiCol_TabHovered}, {"TabActive", ImGuiCol_TabActive},
        {"TabUnfocused", ImGuiCol_TabUnfocused}, {"TabUnfocusedActive", ImGuiCol_TabUnfocusedActive},
        {"TextSelectedBg", ImGuiCol_TextSelectedBg}, {"NavHighlight", ImGuiCol_NavHighlight},
        {"Text", ImGuiCol_Text}, {"TextDisabled", ImGuiCol_TextDisabled},
    };
    return m;
}

static void extract_theme_metadata(int idx, const nlohmann::json& j) {
    if (idx < 0 || idx >= 5) return;
    if (j.contains("name") && j["name"].is_string())
        g_theme_data[idx].name = j["name"].get<std::string>();
    if (j.contains("description") && j["description"].is_string())
        g_theme_data[idx].description = j["description"].get<std::string>();
    if (j.contains("has_palette_editor") && j["has_palette_editor"].is_boolean())
        g_theme_data[idx].has_palette_editor = j["has_palette_editor"].get<bool>();
    if (j.contains("background_image") && j["background_image"].is_string())
        g_theme_data[idx].background_image = j["background_image"].get<std::string>();
    if (j.contains("palette") && j["palette"].is_object()) {
        auto& p = j["palette"];
        if (p.contains("child_bg"))
            g_theme_data[idx].palette_child_bg = json_to_vec4(p["child_bg"], g_theme_data[idx].palette_child_bg);
        if (p.contains("selected_bg"))
            g_theme_data[idx].palette_selected_bg = json_to_vec4(p["selected_bg"], g_theme_data[idx].palette_selected_bg);
        if (p.contains("hovered_bg"))
            g_theme_data[idx].palette_hovered_bg = json_to_vec4(p["hovered_bg"], g_theme_data[idx].palette_hovered_bg);
        if (p.contains("text"))
            g_theme_data[idx].palette_text = json_to_vec4(p["text"], g_theme_data[idx].palette_text);
    }
}

static bool valid_theme_value(const nlohmann::json& value) {
    if (value.is_number()) return true;
    if (!value.is_array() || value.size() < 2) return false;
    for (const auto& component : value)
        if (!component.is_number()) return false;
    return true;
}

static nlohmann::json load_theme_json(int idx, const std::string& base_path) {
    auto result = nlohmann::json::parse(embedded_themes::json[idx]);
    try {
        std::ifstream f(std::filesystem::u8path(base_path + theme_filename(static_cast<AppTheme>(idx))));
        if (!f.is_open()) return result;
        nlohmann::json custom;
        f >> custom;
        if (!custom.is_object()) return result;
        for (const char* key : {"name", "description", "background_image"})
            if (custom.contains(key) && !custom[key].is_string()) return result;
        if (custom.contains("has_palette_editor") && !custom["has_palette_editor"].is_boolean()) return result;
        for (const char* section : {"style", "colors", "palette"}) {
            if (!custom.contains(section)) continue;
            if (!custom[section].is_object()) return result;
            for (const auto& item : custom[section].items()) {
                if (!valid_theme_value(item.value())) return result;
                if (result.contains(section) && result[section].contains(item.key())) {
                    const auto& original = result[section][item.key()];
                    if (original.is_array()) {
                        if (!item.value().is_array() || item.value().size() != original.size()) return result;
                    } else if (!item.value().is_number()) return result;
                }
                if (std::string(section) != "style" && (!item.value().is_array() || item.value().size() != 4)) return result;
            }
        }
        result.merge_patch(custom);
    } catch (...) {}
    return result;
}

void ensure_theme_metadata_loaded() {
    static bool done = false;
    if (done) return;
    done = true;

    for (int i = 0; i < 5; ++i) {
        extract_theme_metadata(i, load_theme_json(i, themes_dir()));
    }
}

bool ui_load_theme_from_file(AppTheme theme, const std::string& base_path) {
    ensure_theme_metadata_loaded();

    int idx = static_cast<int>(theme);
    if (idx < 0 || idx >= 5) return false;

    auto j = load_theme_json(idx, base_path);

    extract_theme_metadata(idx, j);

    ImGuiStyle& st = ImGui::GetStyle();

    if (j.contains("style") && j["style"].is_object()) {
        auto& sj = j["style"];
        auto get_float = [&](const char* key, float& target) {
            if (sj.contains(key) && sj[key].is_number()) target = sj[key].get<float>();
        };
        get_float("WindowRounding", st.WindowRounding);
        get_float("ChildRounding", st.ChildRounding);
        get_float("FrameRounding", st.FrameRounding);
        get_float("PopupRounding", st.PopupRounding);
        get_float("ScrollbarRounding", st.ScrollbarRounding);
        get_float("GrabRounding", st.GrabRounding);
        get_float("TabRounding", st.TabRounding);
        get_float("WindowBorderSize", st.WindowBorderSize);
        get_float("ChildBorderSize", st.ChildBorderSize);
        get_float("PopupBorderSize", st.PopupBorderSize);
        get_float("FrameBorderSize", st.FrameBorderSize);
        get_float("ScrollbarSize", st.ScrollbarSize);
        get_float("GrabMinSize", st.GrabMinSize);
        get_float("IndentSpacing", st.IndentSpacing);

        if (sj.contains("WindowPadding")) st.WindowPadding = json_to_vec2(sj["WindowPadding"], st.WindowPadding);
        if (sj.contains("FramePadding")) st.FramePadding = json_to_vec2(sj["FramePadding"], st.FramePadding);
        if (sj.contains("ItemSpacing")) st.ItemSpacing = json_to_vec2(sj["ItemSpacing"], st.ItemSpacing);
        if (sj.contains("ItemInnerSpacing")) st.ItemInnerSpacing = json_to_vec2(sj["ItemInnerSpacing"], st.ItemInnerSpacing);
        if (sj.contains("WindowMinSize")) st.WindowMinSize = json_to_vec2(sj["WindowMinSize"], st.WindowMinSize);
    }

    if (j.contains("colors") && j["colors"].is_object()) {
        const auto& cmap = color_name_map();
        for (auto& [key, value] : j["colors"].items()) {
            auto it = cmap.find(key);
            if (it != cmap.end()) {
                st.Colors[it->second] = json_to_vec4(value, st.Colors[it->second]);
            }
        }
    }

    return true;
}

void ui_get_theme_palette(AppTheme theme, ImVec4& child_bg, ImVec4& selected_bg, ImVec4& hovered_bg, ImVec4& text) {
    ensure_theme_metadata_loaded();
    int idx = static_cast<int>(theme);
    if (idx < 0 || idx >= 5) {
        child_bg = {}; selected_bg = {}; hovered_bg = {}; text = {};
        return;
    }
    child_bg = g_theme_data[idx].palette_child_bg;
    selected_bg = g_theme_data[idx].palette_selected_bg;
    hovered_bg = g_theme_data[idx].palette_hovered_bg;
    text = g_theme_data[idx].palette_text;
}

bool ui_theme_has_palette_editor(AppTheme theme) {
    ensure_theme_metadata_loaded();
    int idx = static_cast<int>(theme);
    if (idx < 0 || idx >= 5) return false;
    return g_theme_data[idx].has_palette_editor;
}

const char* ui_theme_background_image(AppTheme theme) {
    ensure_theme_metadata_loaded();
    int idx = static_cast<int>(theme);
    if (idx < 0 || idx >= 5 || g_theme_data[idx].background_image.empty()) return nullptr;
    return g_theme_data[idx].background_image.c_str();
}

static void load_theme_background(AppTheme t) {
    if (g_bg_crimson_tex != (ImTextureID)0) return;

    int idx = static_cast<int>(t);
    if (idx < 0 || idx >= 5 || g_theme_data[idx].background_image.empty()) return;
    std::ifstream file(std::filesystem::u8path(themes_dir() + g_theme_data[idx].background_image), std::ios::binary);
    const std::string bytes{std::istreambuf_iterator<char>(file), {}};

    int w = 0, h = 0, ch = 0;
    unsigned char* pixels = bytes.empty() || bytes.size() > (32u << 20) ? nullptr :
        stbi_load_from_memory(reinterpret_cast<const unsigned char*>(bytes.data()), static_cast<int>(bytes.size()), &w, &h, &ch, 4);
    if (!pixels)
        pixels = stbi_load_from_memory(embedded_themes::background,
            static_cast<int>(embedded_themes::background_size), &w, &h, &ch, 4);
    if (!pixels) return;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);
    g_bg_crimson_tex = (ImTextureID)(void*)(uintptr_t)tex;
}

void ui_shutdown_theme() {
    if (g_bg_crimson_tex == (ImTextureID)0) return;
    GLuint texture = (GLuint)(uintptr_t)(void*)g_bg_crimson_tex;
    glDeleteTextures(1, &texture);
    g_bg_crimson_tex = (ImTextureID)0;
}

void ui_apply_theme(AppTheme t) {
    ImGui::StyleColorsDark();
    g_current_theme = t;
    g_theme = static_cast<int>(t);

    ui_load_theme_from_file(t, themes_dir());

    if (ui_theme_background_image(t)) {
        load_theme_background(t);
    }

    if (ui_theme_has_palette_editor(t)) {
        ui_get_theme_palette(t, g_crimson_child_bg, g_crimson_selected_bg, g_crimson_hovered_bg, g_crimson_text);
    }
}
