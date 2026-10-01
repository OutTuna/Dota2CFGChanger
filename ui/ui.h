#pragma once
#include "imgui.h"
#include <string>

enum class AppTheme { Dark = 0, Indigo, Vermillion, ClassicSteam, Crimson };

void ui_apply_theme(AppTheme t = AppTheme::Dark);
void ui_render_main(ImFont* font_big, ImVec2 ds);
void ui_render_success_popup(ImFont* font_big, ImVec2 ds, float delta_time);

struct ThemeData {
    std::string name;
    int id = 0;
    std::string description;
    bool has_palette_editor = false;
    std::string background_image;
    ImVec4 palette_child_bg = {0.12f, 0.05f, 0.05f, 1.00f};
    ImVec4 palette_selected_bg = {0.55f, 0.08f, 0.08f, 1.00f};
    ImVec4 palette_hovered_bg = {0.30f, 0.06f, 0.06f, 1.00f};
    ImVec4 palette_text = {0.95f, 0.88f, 0.88f, 1.00f};
};

bool ui_load_theme_from_file(AppTheme theme, const std::string& base_path);
void ui_get_theme_palette(AppTheme theme, ImVec4& child_bg, ImVec4& selected_bg, ImVec4& hovered_bg, ImVec4& text);
bool ui_theme_has_palette_editor(AppTheme theme);
const char* ui_theme_background_image(AppTheme theme);
