#include "ui_internal.h"
#include "app.h"

bool g_settings_open = false;
bool g_palette_open = false;
bool g_confirm_copy_open = false;
std::string g_confirm_src_label;
std::string g_confirm_dst_label;

void render_settings_panel(ImVec2 ds) {
    ensure_theme_metadata_loaded();

    ImGui::SetNextWindowPos({ ds.x - 210.f, 34.f });
    ImGui::SetNextWindowSize({ 200.f, 0.f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 12.f, 10.f });

    ImGui::Begin("##settings_panel", &g_settings_open,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextDisabled("Тема оформления");
    ImGui::Spacing();

    static const AppTheme themes[] = {
        AppTheme::Dark, AppTheme::Indigo, AppTheme::Vermillion,
        AppTheme::ClassicSteam, AppTheme::Crimson,
    };

    for (AppTheme id : themes) {
        int idx = static_cast<int>(id);
        const std::string& label = g_theme_data[idx].name;
        bool sel = (g_current_theme == id);
        ImGui::PushStyleColor(ImGuiCol_Text,
            sel ? ImVec4{ 0.92f, 0.92f, 0.94f, 1.f }
                : ImVec4{ 0.55f, 0.55f, 0.60f, 1.f });
        if (ImGui::Selectable(label.c_str(), sel)) {
            ui_apply_theme(id);
            g_settings_open = false;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered() && !g_theme_data[idx].description.empty()) {
            ImGui::SetTooltip("%s", g_theme_data[idx].description.c_str());
        }
    }

    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
        ImGui::IsMouseClicked(0)) {
        g_settings_open = false;
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
}

void render_palette_panel(ImVec2 ds) {
    int idx = static_cast<int>(g_current_theme);
    std::string title = "Цвета списков (" + g_theme_data[idx].name + ")";

    ImGui::SetNextWindowPos({ ds.x - 250.f, 34.f });
    ImGui::SetNextWindowSize({ 238.f, 0.f });
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.05f, 0.05f, 0.05f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_Border, { 0.30f, 0.08f, 0.08f, 1.00f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 12.f, 10.f });

    ImGui::Begin("##palette_panel", &g_palette_open,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextDisabled("%s", title.c_str());
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Фон строки");
    ImGui::ColorEdit4("##child_bg", (float*)&g_crimson_child_bg,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::Spacing();

    ImGui::Text("Выделение");
    ImGui::ColorEdit4("##selected_bg", (float*)&g_crimson_selected_bg,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::Spacing();

    ImGui::Text("Hover");
    ImGui::ColorEdit4("##hovered_bg", (float*)&g_crimson_hovered_bg,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::Spacing();

    ImGui::Text("Текст");
    ImGui::ColorEdit4("##text_col", (float*)&g_crimson_text,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Сбросить", { -1, 0 })) {
        ui_get_theme_palette(g_current_theme, g_crimson_child_bg, g_crimson_selected_bg, g_crimson_hovered_bg, g_crimson_text);
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

void render_confirm_copy_popup(ImVec2 ds) {
    const float POP_W = 380.f, POP_H = 0.f;
    ImGui::SetNextWindowPos({ (ds.x - POP_W) * 0.5f, ds.y * 0.35f });
    ImGui::SetNextWindowSize({ POP_W, POP_H });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 16.f, 14.f });

    ImGui::Begin("##confirm_copy", &g_confirm_copy_open,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextColored({ 0.90f, 0.75f, 0.25f, 1.f }, "Подтверждение");
    ImGui::Spacing();
    ImGui::TextWrapped(
        "Текущий конфиг Dota 2 в целевом аккаунте будет заменён. "
        "Отменить это можно только вручную, из собственной резервной копии, если она у вас есть.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextDisabled("Откуда:");
    ImGui::TextWrapped("%s", g_confirm_src_label.c_str());
    ImGui::TextDisabled("Куда:");
    ImGui::TextWrapped("%s", g_confirm_dst_label.c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Отмена", { 120, 0 })) {
        g_confirm_copy_open = false;
    }
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, { 0.75f, 0.20f, 0.20f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.85f, 0.25f, 0.25f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.60f, 0.14f, 0.14f, 1.00f });
    if (ImGui::Button("Заменить конфиг", { -1, 0 })) {
        copy_config();
        g_confirm_copy_open = false;
    }
    ImGui::PopStyleColor(3);

    ImGui::End();
    ImGui::PopStyleVar(2);
}

