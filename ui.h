#pragma once
#include "imgui.h"

enum class AppTheme { Dark = 0, Indigo, Vermillion, ClassicSteam, Crimson };

void ui_apply_theme(AppTheme t = AppTheme::Dark);
void ui_render_main(ImFont* font_big, ImVec2 ds);
void ui_render_success_popup(ImFont* font_big, ImVec2 ds, float delta_time);