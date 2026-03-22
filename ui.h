#pragma once
#include "imgui.h"

void ui_apply_theme();
void ui_render_main(ImFont* font_big, ImVec2 ds);
void ui_render_success_popup(ImFont* font_big, ImVec2 ds, float delta_time);