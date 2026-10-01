#pragma once
#include "ui.h"
#include <imgui.h>
#include <string>

extern AppTheme g_current_theme;

extern ImTextureID g_bg_crimson_tex;
extern ImVec4 g_crimson_child_bg;
extern ImVec4 g_crimson_selected_bg;
extern ImVec4 g_crimson_hovered_bg;
extern ImVec4 g_crimson_text;
extern ThemeData g_theme_data[5];

extern bool g_palette_open;
extern bool g_confirm_copy_open;
extern std::string g_confirm_src_label;
extern std::string g_confirm_dst_label;

std::string themes_dir();
void ensure_theme_metadata_loaded();

void render_palette_panel(ImVec2 ds);
void render_confirm_copy_popup(ImVec2 ds);
