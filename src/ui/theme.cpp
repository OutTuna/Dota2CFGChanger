#include "ui_internal.h"
#include "app.h"
#include "embedded_themes.h"
#include "stb_image.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
#include <fstream>
#include <unordered_map>
#include <nlohmann/json.hpp>

AppTheme g_current_theme = AppTheme::Dark;

ImTextureID g_bg_crimson_tex = (ImTextureID)0;

ImVec4 g_crimson_child_bg = { 0.12f, 0.05f, 0.05f, 1.00f };
ImVec4 g_crimson_selected_bg = { 0.55f, 0.08f, 0.08f, 1.00f };
ImVec4 g_crimson_hovered_bg = { 0.30f, 0.06f, 0.06f, 1.00f };
ImVec4 g_crimson_text = { 0.95f, 0.88f, 0.88f, 1.00f };

std::string themes_dir() { return exe_dir() + "/themes/"; }

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
        std::ifstream f(base_path + theme_filename(static_cast<AppTheme>(idx)));
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

static void apply_dark_builtin(ImGuiStyle& st) {
    st.WindowRounding = 8.f;
    st.ChildRounding = 6.f;
    st.FrameRounding = 5.f;
    st.PopupRounding = 6.f;
    st.ScrollbarRounding = 5.f;
    st.GrabRounding = 4.f;
    st.TabRounding = 5.f;
    st.WindowBorderSize = 1.f;
    st.FrameBorderSize = 0.f;
    st.WindowPadding = { 14.f, 14.f };
    st.FramePadding = { 10.f, 6.f };
    st.ItemSpacing = { 8.f, 5.f };
    st.ItemInnerSpacing = { 6.f, 3.f };
    st.ScrollbarSize = 10.f;
    st.GrabMinSize = 8.f;
    st.IndentSpacing = 18.f;

    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg] = { 0.09f, 0.09f, 0.10f, 1.00f };
    c[ImGuiCol_ChildBg] = { 0.11f, 0.11f, 0.13f, 1.00f };
    c[ImGuiCol_PopupBg] = { 0.10f, 0.10f, 0.12f, 1.00f };
    c[ImGuiCol_Border] = { 0.22f, 0.22f, 0.26f, 1.00f };
    c[ImGuiCol_BorderShadow] = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_FrameBg] = { 0.15f, 0.15f, 0.18f, 1.00f };
    c[ImGuiCol_FrameBgHovered] = { 0.28f, 0.28f, 0.32f, 1.00f };
    c[ImGuiCol_FrameBgActive] = { 0.24f, 0.24f, 0.28f, 1.00f };
    c[ImGuiCol_TitleBg] = { 0.07f, 0.07f, 0.08f, 1.00f };
    c[ImGuiCol_TitleBgActive] = { 0.07f, 0.07f, 0.08f, 1.00f };
    c[ImGuiCol_TitleBgCollapsed] = { 0.07f, 0.07f, 0.08f, 1.00f };
    c[ImGuiCol_ScrollbarBg] = { 0.09f, 0.09f, 0.10f, 1.00f };
    c[ImGuiCol_ScrollbarGrab] = { 0.28f, 0.28f, 0.32f, 1.00f };
    c[ImGuiCol_ScrollbarGrabHovered] = { 0.38f, 0.38f, 0.44f, 1.00f };
    c[ImGuiCol_ScrollbarGrabActive] = { 0.50f, 0.50f, 0.58f, 1.00f };
    c[ImGuiCol_CheckMark] = { 0.90f, 0.25f, 0.25f, 1.00f };
    c[ImGuiCol_SliderGrab] = { 0.80f, 0.22f, 0.22f, 1.00f };
    c[ImGuiCol_SliderGrabActive] = { 1.00f, 0.30f, 0.30f, 1.00f };
    c[ImGuiCol_Button] = { 0.20f, 0.20f, 0.24f, 1.00f };
    c[ImGuiCol_ButtonHovered] = { 0.75f, 0.20f, 0.20f, 1.00f };
    c[ImGuiCol_ButtonActive] = { 0.55f, 0.14f, 0.14f, 1.00f };
    c[ImGuiCol_Header] = { 0.75f, 0.20f, 0.20f, 0.40f };
    c[ImGuiCol_HeaderHovered] = { 0.75f, 0.20f, 0.20f, 0.65f };
    c[ImGuiCol_HeaderActive] = { 0.75f, 0.20f, 0.20f, 0.90f };
    c[ImGuiCol_Separator] = { 0.22f, 0.22f, 0.26f, 1.00f };
    c[ImGuiCol_SeparatorHovered] = { 0.75f, 0.20f, 0.20f, 0.70f };
    c[ImGuiCol_SeparatorActive] = { 0.75f, 0.20f, 0.20f, 1.00f };
    c[ImGuiCol_ResizeGrip] = { 0.75f, 0.20f, 0.20f, 0.20f };
    c[ImGuiCol_ResizeGripHovered] = { 0.75f, 0.20f, 0.20f, 0.60f };
    c[ImGuiCol_ResizeGripActive] = { 0.75f, 0.20f, 0.20f, 0.90f };
    c[ImGuiCol_Tab] = { 0.15f, 0.15f, 0.18f, 1.00f };
    c[ImGuiCol_TabHovered] = { 0.75f, 0.20f, 0.20f, 0.80f };
    c[ImGuiCol_TabActive] = { 0.60f, 0.16f, 0.16f, 1.00f };
    c[ImGuiCol_TabUnfocused] = { 0.12f, 0.12f, 0.14f, 1.00f };
    c[ImGuiCol_TabUnfocusedActive] = { 0.30f, 0.14f, 0.14f, 1.00f };
    c[ImGuiCol_TextSelectedBg] = { 0.75f, 0.20f, 0.20f, 0.35f };
    c[ImGuiCol_NavHighlight] = { 0.75f, 0.20f, 0.20f, 1.00f };
    c[ImGuiCol_Text] = { 0.92f, 0.92f, 0.94f, 1.00f };
    c[ImGuiCol_TextDisabled] = { 0.45f, 0.45f, 0.50f, 1.00f };
}

static void apply_indigo_builtin(ImGuiStyle& st) {
    const float r = 2.f;
    st.WindowBorderSize = 1.f;
    st.FrameBorderSize = 1.f;
    st.WindowMinSize = { 75.f, 50.f };
    st.FramePadding = { 5.f, 5.f };
    st.ItemSpacing = { 6.f, 5.f };
    st.ItemInnerSpacing = { 2.f, 4.f };
    st.WindowRounding = 0.f;
    st.FrameRounding = r;
    st.PopupRounding = 0.f;
    st.PopupBorderSize = 1.f;
    st.IndentSpacing = 6.f;
    st.GrabMinSize = 14.f;
    st.GrabRounding = r;
    st.ScrollbarSize = 12.f;
    st.ScrollbarRounding = r;

    ImVec4* c = st.Colors;
    c[ImGuiCol_Text] = { 1.00f, 1.00f, 1.00f, 1.00f };
    c[ImGuiCol_TextDisabled] = { 0.50f, 0.50f, 0.50f, 1.00f };
    c[ImGuiCol_WindowBg] = { 0.20f, 0.23f, 0.31f, 1.00f };
    c[ImGuiCol_ChildBg] = { 0.20f, 0.23f, 0.31f, 1.00f };
    c[ImGuiCol_PopupBg] = { 0.20f, 0.23f, 0.31f, 1.00f };
    c[ImGuiCol_Border] = { 0.00f, 0.00f, 0.00f, 1.00f };
    c[ImGuiCol_BorderShadow] = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_FrameBg] = { 0.25f, 0.28f, 0.38f, 1.00f };
    c[ImGuiCol_FrameBgHovered] = { 0.25f, 0.28f, 0.38f, 1.00f };
    c[ImGuiCol_FrameBgActive] = { 0.25f, 0.28f, 0.38f, 1.00f };
    c[ImGuiCol_TitleBg] = { 0.00f, 0.43f, 1.00f, 1.00f };
    c[ImGuiCol_TitleBgActive] = { 0.00f, 0.55f, 1.00f, 1.00f };
    c[ImGuiCol_TitleBgCollapsed] = { 0.10f, 0.69f, 1.00f, 1.00f };
    c[ImGuiCol_MenuBarBg] = { 0.25f, 0.28f, 0.38f, 1.00f };
    c[ImGuiCol_ScrollbarBg] = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_ScrollbarGrab] = { 0.39f, 0.44f, 0.56f, 1.00f };
    c[ImGuiCol_ScrollbarGrabHovered] = { 0.12f, 0.43f, 1.00f, 1.00f };
    c[ImGuiCol_ScrollbarGrabActive] = { 0.00f, 0.55f, 1.00f, 1.00f };
    c[ImGuiCol_CheckMark] = { 0.00f, 0.55f, 1.00f, 1.00f };
    c[ImGuiCol_SliderGrab] = { 0.00f, 0.55f, 1.00f, 1.00f };
    c[ImGuiCol_SliderGrabActive] = { 0.10f, 0.69f, 1.00f, 1.00f };
    c[ImGuiCol_Button] = { 0.25f, 0.28f, 0.38f, 1.00f };
    c[ImGuiCol_ButtonHovered] = { 0.12f, 0.43f, 1.00f, 1.00f };
    c[ImGuiCol_ButtonActive] = { 0.00f, 0.55f, 1.00f, 1.00f };
    c[ImGuiCol_Header] = { 0.00f, 0.43f, 1.00f, 1.00f };
    c[ImGuiCol_HeaderHovered] = { 0.00f, 0.55f, 1.00f, 1.00f };
    c[ImGuiCol_HeaderActive] = { 0.00f, 0.43f, 1.00f, 1.00f };
    c[ImGuiCol_Separator] = { 0.43f, 0.43f, 0.50f, 0.50f };
    c[ImGuiCol_SeparatorHovered] = { 0.10f, 0.40f, 0.75f, 0.78f };
    c[ImGuiCol_SeparatorActive] = { 0.10f, 0.40f, 0.75f, 1.00f };
    c[ImGuiCol_ResizeGrip] = { 0.26f, 0.59f, 0.98f, 0.25f };
    c[ImGuiCol_ResizeGripHovered] = { 0.26f, 0.59f, 0.98f, 0.67f };
    c[ImGuiCol_ResizeGripActive] = { 0.26f, 0.59f, 0.98f, 0.95f };
    c[ImGuiCol_Tab] = { 0.00f, 0.50f, 1.00f, 1.00f };
    c[ImGuiCol_TabHovered] = { 0.12f, 0.69f, 1.00f, 1.00f };
    c[ImGuiCol_TabActive] = { 0.12f, 0.69f, 1.00f, 1.00f };
    c[ImGuiCol_TabUnfocused] = { 0.07f, 0.10f, 0.15f, 0.97f };
    c[ImGuiCol_TabUnfocusedActive] = { 0.14f, 0.26f, 0.42f, 1.00f };
    c[ImGuiCol_TextSelectedBg] = { 0.26f, 0.59f, 0.98f, 0.35f };
    c[ImGuiCol_NavHighlight] = { 0.26f, 0.59f, 0.98f, 1.00f };
}

static void apply_vermillion_builtin(ImGuiStyle& st) {
    const float r = 2.f;
    st.WindowBorderSize = 1.f;
    st.FrameBorderSize = 1.f;
    st.WindowMinSize = { 75.f, 50.f };
    st.FramePadding = { 5.f, 5.f };
    st.ItemSpacing = { 6.f, 5.f };
    st.ItemInnerSpacing = { 2.f, 4.f };
    st.WindowRounding = 0.f;
    st.FrameRounding = r;
    st.PopupRounding = 0.f;
    st.GrabMinSize = 14.f;
    st.GrabRounding = r;
    st.ScrollbarSize = 12.f;
    st.ScrollbarRounding = r;

    ImVec4* c = st.Colors;
    c[ImGuiCol_Text] = { 1.00f, 1.00f, 1.00f, 0.75f };
    c[ImGuiCol_TextDisabled] = { 1.00f, 0.18f, 0.29f, 0.78f };
    c[ImGuiCol_WindowBg] = { 0.17f, 0.20f, 0.25f, 1.00f };
    c[ImGuiCol_ChildBg] = { 0.20f, 0.22f, 0.27f, 0.57f };
    c[ImGuiCol_PopupBg] = { 0.17f, 0.20f, 0.25f, 1.00f };
    c[ImGuiCol_Border] = { 0.00f, 0.00f, 0.00f, 1.00f };
    c[ImGuiCol_BorderShadow] = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_FrameBg] = { 0.22f, 0.25f, 0.31f, 1.00f };
    c[ImGuiCol_FrameBgHovered] = { 0.22f, 0.25f, 0.31f, 1.00f };
    c[ImGuiCol_FrameBgActive] = { 0.22f, 0.25f, 0.31f, 1.00f };
    c[ImGuiCol_TitleBg] = { 0.65f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_TitleBgActive] = { 0.78f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_TitleBgCollapsed] = { 0.78f, 0.18f, 0.29f, 0.60f };
    c[ImGuiCol_ScrollbarBg] = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_ScrollbarGrab] = { 0.65f, 0.18f, 0.29f, 0.37f };
    c[ImGuiCol_ScrollbarGrabHovered] = { 0.78f, 0.18f, 0.29f, 0.78f };
    c[ImGuiCol_ScrollbarGrabActive] = { 0.78f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_CheckMark] = { 0.71f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_SliderGrab] = { 0.78f, 0.18f, 0.29f, 0.37f };
    c[ImGuiCol_SliderGrabActive] = { 0.92f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_Button] = { 0.65f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_ButtonHovered] = { 0.78f, 0.18f, 0.29f, 0.86f };
    c[ImGuiCol_ButtonActive] = { 0.78f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_Header] = { 0.78f, 0.18f, 0.29f, 0.76f };
    c[ImGuiCol_HeaderHovered] = { 0.78f, 0.18f, 0.29f, 0.86f };
    c[ImGuiCol_HeaderActive] = { 0.78f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_Separator] = { 0.15f, 0.00f, 0.00f, 0.35f };
    c[ImGuiCol_SeparatorHovered] = { 0.78f, 0.18f, 0.29f, 0.59f };
    c[ImGuiCol_SeparatorActive] = { 0.78f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_ResizeGrip] = { 0.78f, 0.18f, 0.29f, 0.63f };
    c[ImGuiCol_ResizeGripHovered] = { 0.78f, 0.18f, 0.29f, 0.78f };
    c[ImGuiCol_ResizeGripActive] = { 0.78f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_Tab] = { 0.78f, 0.18f, 0.29f, 0.76f };
    c[ImGuiCol_TabHovered] = { 0.78f, 0.18f, 0.29f, 0.86f };
    c[ImGuiCol_TabActive] = { 0.78f, 0.18f, 0.29f, 1.00f };
    c[ImGuiCol_TabUnfocused] = { 0.07f, 0.10f, 0.15f, 0.97f };
    c[ImGuiCol_TabUnfocusedActive] = { 0.14f, 0.26f, 0.42f, 1.00f };
    c[ImGuiCol_TextSelectedBg] = { 0.92f, 0.18f, 0.29f, 0.43f };
    c[ImGuiCol_NavHighlight] = { 0.45f, 0.45f, 0.90f, 0.80f };
}

static void apply_classic_steam_builtin(ImGuiStyle& st) {
    st.WindowRounding = 0.f;
    st.ChildRounding = 0.f;
    st.FrameRounding = 0.f;
    st.PopupRounding = 0.f;
    st.ScrollbarRounding = 0.f;
    st.GrabRounding = 0.f;
    st.TabRounding = 0.f;
    st.WindowBorderSize = 1.f;
    st.ChildBorderSize = 1.f;
    st.PopupBorderSize = 1.f;
    st.FrameBorderSize = 1.f;
    st.WindowPadding = { 8.f, 8.f };
    st.FramePadding = { 4.f, 3.f };
    st.ItemSpacing = { 8.f, 4.f };
    st.ItemInnerSpacing = { 4.f, 4.f };
    st.IndentSpacing = 21.f;
    st.ScrollbarSize = 14.f;
    st.GrabMinSize = 10.f;

    ImVec4* c = st.Colors;
    c[ImGuiCol_Text] = { 1.000f, 1.000f, 1.000f, 1.00f };
    c[ImGuiCol_TextDisabled] = { 0.498f, 0.498f, 0.498f, 1.00f };
    c[ImGuiCol_WindowBg] = { 0.286f, 0.337f, 0.259f, 1.00f };
    c[ImGuiCol_ChildBg] = { 0.286f, 0.337f, 0.259f, 1.00f };
    c[ImGuiCol_PopupBg] = { 0.239f, 0.267f, 0.200f, 1.00f };
    c[ImGuiCol_Border] = { 0.537f, 0.569f, 0.510f, 0.50f };
    c[ImGuiCol_BorderShadow] = { 0.137f, 0.157f, 0.110f, 0.52f };
    c[ImGuiCol_FrameBg] = { 0.239f, 0.267f, 0.200f, 1.00f };
    c[ImGuiCol_FrameBgHovered] = { 0.267f, 0.298f, 0.227f, 1.00f };
    c[ImGuiCol_FrameBgActive] = { 0.298f, 0.337f, 0.259f, 1.00f };
    c[ImGuiCol_TitleBg] = { 0.239f, 0.267f, 0.200f, 1.00f };
    c[ImGuiCol_TitleBgActive] = { 0.286f, 0.337f, 0.259f, 1.00f };
    c[ImGuiCol_TitleBgCollapsed] = { 0.000f, 0.000f, 0.000f, 0.51f };
    c[ImGuiCol_ScrollbarBg] = { 0.349f, 0.420f, 0.310f, 1.00f };
    c[ImGuiCol_ScrollbarGrab] = { 0.278f, 0.318f, 0.239f, 1.00f };
    c[ImGuiCol_ScrollbarGrabHovered] = { 0.247f, 0.298f, 0.220f, 1.00f };
    c[ImGuiCol_ScrollbarGrabActive] = { 0.227f, 0.267f, 0.208f, 1.00f };
    c[ImGuiCol_CheckMark] = { 0.588f, 0.537f, 0.176f, 1.00f };
    c[ImGuiCol_SliderGrab] = { 0.349f, 0.420f, 0.310f, 1.00f };
    c[ImGuiCol_SliderGrabActive] = { 0.537f, 0.569f, 0.510f, 0.50f };
    c[ImGuiCol_Button] = { 0.286f, 0.337f, 0.259f, 0.40f };
    c[ImGuiCol_ButtonHovered] = { 0.349f, 0.420f, 0.310f, 1.00f };
    c[ImGuiCol_ButtonActive] = { 0.537f, 0.569f, 0.510f, 0.50f };
    c[ImGuiCol_Header] = { 0.349f, 0.420f, 0.310f, 1.00f };
    c[ImGuiCol_HeaderHovered] = { 0.349f, 0.420f, 0.310f, 0.60f };
    c[ImGuiCol_HeaderActive] = { 0.537f, 0.569f, 0.510f, 0.50f };
    c[ImGuiCol_Separator] = { 0.137f, 0.157f, 0.110f, 1.00f };
    c[ImGuiCol_SeparatorHovered] = { 0.537f, 0.569f, 0.510f, 1.00f };
    c[ImGuiCol_SeparatorActive] = { 0.588f, 0.537f, 0.176f, 1.00f };
    c[ImGuiCol_Tab] = { 0.349f, 0.420f, 0.310f, 1.00f };
    c[ImGuiCol_TabHovered] = { 0.537f, 0.569f, 0.510f, 0.78f };
    c[ImGuiCol_TabActive] = { 0.588f, 0.537f, 0.176f, 1.00f };
    c[ImGuiCol_TabUnfocused] = { 0.239f, 0.267f, 0.200f, 1.00f };
    c[ImGuiCol_TabUnfocusedActive] = { 0.349f, 0.420f, 0.310f, 1.00f };
    c[ImGuiCol_TextSelectedBg] = { 0.588f, 0.537f, 0.176f, 1.00f };
    c[ImGuiCol_NavHighlight] = { 0.588f, 0.537f, 0.176f, 1.00f };
}

static void apply_crimson_builtin(ImGuiStyle& st) {
    st.WindowRounding = 4.f;
    st.ChildRounding = 3.f;
    st.FrameRounding = 3.f;
    st.PopupRounding = 4.f;
    st.ScrollbarRounding = 3.f;
    st.GrabRounding = 3.f;
    st.TabRounding = 3.f;
    st.WindowBorderSize = 1.f;
    st.FrameBorderSize = 0.f;
    st.WindowPadding = { 14.f, 14.f };
    st.FramePadding = { 10.f, 6.f };
    st.ItemSpacing = { 8.f, 5.f };
    st.ItemInnerSpacing = { 6.f, 3.f };
    st.ScrollbarSize = 10.f;
    st.GrabMinSize = 8.f;
    st.IndentSpacing = 18.f;

    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg] = { 0.06f, 0.04f, 0.04f, 0.00f };
    c[ImGuiCol_ChildBg] = { 0.08f, 0.05f, 0.05f, 1.00f };
    c[ImGuiCol_PopupBg] = { 0.07f, 0.04f, 0.04f, 1.00f };
    c[ImGuiCol_Border] = { 0.25f, 0.08f, 0.08f, 1.00f };
    c[ImGuiCol_BorderShadow] = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_FrameBg] = { 0.12f, 0.07f, 0.07f, 1.00f };
    c[ImGuiCol_FrameBgHovered] = { 0.18f, 0.09f, 0.09f, 1.00f };
    c[ImGuiCol_FrameBgActive] = { 0.22f, 0.10f, 0.10f, 1.00f };
    c[ImGuiCol_TitleBg] = { 0.08f, 0.04f, 0.04f, 1.00f };
    c[ImGuiCol_TitleBgActive] = { 0.10f, 0.05f, 0.05f, 1.00f };
    c[ImGuiCol_TitleBgCollapsed] = { 0.06f, 0.03f, 0.03f, 1.00f };
    c[ImGuiCol_ScrollbarBg] = { 0.06f, 0.04f, 0.04f, 1.00f };
    c[ImGuiCol_ScrollbarGrab] = { 0.30f, 0.08f, 0.08f, 1.00f };
    c[ImGuiCol_ScrollbarGrabHovered] = { 0.45f, 0.10f, 0.10f, 1.00f };
    c[ImGuiCol_ScrollbarGrabActive] = { 0.60f, 0.12f, 0.12f, 1.00f };
    c[ImGuiCol_CheckMark] = { 0.85f, 0.15f, 0.15f, 1.00f };
    c[ImGuiCol_SliderGrab] = { 0.70f, 0.12f, 0.12f, 1.00f };
    c[ImGuiCol_SliderGrabActive] = { 0.90f, 0.18f, 0.18f, 1.00f };
    c[ImGuiCol_Button] = { 0.18f, 0.07f, 0.07f, 1.00f };
    c[ImGuiCol_ButtonHovered] = { 0.55f, 0.10f, 0.10f, 1.00f };
    c[ImGuiCol_ButtonActive] = { 0.72f, 0.14f, 0.14f, 1.00f };
    c[ImGuiCol_Header] = { 0.45f, 0.08f, 0.08f, 0.55f };
    c[ImGuiCol_HeaderHovered] = { 0.55f, 0.10f, 0.10f, 0.75f };
    c[ImGuiCol_HeaderActive] = { 0.70f, 0.14f, 0.14f, 1.00f };
    c[ImGuiCol_Separator] = { 0.22f, 0.07f, 0.07f, 1.00f };
    c[ImGuiCol_SeparatorHovered] = { 0.55f, 0.10f, 0.10f, 0.80f };
    c[ImGuiCol_SeparatorActive] = { 0.72f, 0.14f, 0.14f, 1.00f };
    c[ImGuiCol_ResizeGrip] = { 0.45f, 0.08f, 0.08f, 0.25f };
    c[ImGuiCol_ResizeGripHovered] = { 0.60f, 0.10f, 0.10f, 0.65f };
    c[ImGuiCol_ResizeGripActive] = { 0.75f, 0.14f, 0.14f, 0.95f };
    c[ImGuiCol_Tab] = { 0.14f, 0.06f, 0.06f, 1.00f };
    c[ImGuiCol_TabHovered] = { 0.55f, 0.10f, 0.10f, 0.85f };
    c[ImGuiCol_TabActive] = { 0.45f, 0.08f, 0.08f, 1.00f };
    c[ImGuiCol_TabUnfocused] = { 0.08f, 0.04f, 0.04f, 1.00f };
    c[ImGuiCol_TabUnfocusedActive] = { 0.18f, 0.07f, 0.07f, 1.00f };
    c[ImGuiCol_TextSelectedBg] = { 0.60f, 0.10f, 0.10f, 0.40f };
    c[ImGuiCol_NavHighlight] = { 0.72f, 0.14f, 0.14f, 1.00f };
    c[ImGuiCol_Text] = { 0.92f, 0.88f, 0.88f, 1.00f };
    c[ImGuiCol_TextDisabled] = { 0.40f, 0.28f, 0.28f, 1.00f };
}

static void load_theme_background(AppTheme t) {
    if (g_bg_crimson_tex != (ImTextureID)0) return;

    int idx = static_cast<int>(t);
    if (idx < 0 || idx >= 5 || g_theme_data[idx].background_image.empty()) return;
    std::string path = themes_dir() + g_theme_data[idx].background_image;

    int w = 0, h = 0, ch = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &ch, 4);
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

void ui_apply_theme(AppTheme t) {
    ImGui::StyleColorsDark();
    g_current_theme = t;
    g_theme = static_cast<int>(t);
    save_settings();

    if (!ui_load_theme_from_file(t, themes_dir())) {
        ImGuiStyle& st = ImGui::GetStyle();
        switch (t) {
        case AppTheme::Indigo: apply_indigo_builtin(st); break;
        case AppTheme::Vermillion: apply_vermillion_builtin(st); break;
        case AppTheme::ClassicSteam: apply_classic_steam_builtin(st); break;
        case AppTheme::Crimson: apply_crimson_builtin(st); break;
        default: apply_dark_builtin(st); break;
        }
    }

    if (ui_theme_background_image(t)) {
        load_theme_background(t);
    }

    if (ui_theme_has_palette_editor(t)) {
        ui_get_theme_palette(t, g_crimson_child_bg, g_crimson_selected_bg, g_crimson_hovered_bg, g_crimson_text);
    }
}
