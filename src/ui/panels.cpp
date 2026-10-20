#include "ui_internal.h"
#include "app.h"
#include "localization.h"
#include "updates.h"

bool g_palette_open = false;
bool g_confirm_copy_open = false;
std::string g_confirm_src_label;
std::string g_confirm_dst_label;

void render_palette_panel(ImVec2 ds) {
    int idx = static_cast<int>(g_current_theme);

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

    ImGui::TextDisabled("%s (%s)", tr("palette"), g_theme_data[idx].name.c_str());
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted(tr("row_background"));
    ImGui::ColorEdit4("##child_bg", (float*)&g_crimson_child_bg,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::Spacing();

    ImGui::TextUnformatted(tr("selection"));
    ImGui::ColorEdit4("##selected_bg", (float*)&g_crimson_selected_bg,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::Spacing();

    ImGui::TextUnformatted(tr("hover"));
    ImGui::ColorEdit4("##hovered_bg", (float*)&g_crimson_hovered_bg,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::Spacing();

    ImGui::TextUnformatted(tr("text"));
    ImGui::ColorEdit4("##text_col", (float*)&g_crimson_text,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button(tr("reset"), { -1, 0 })) {
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
    const float POP_W = 540.f;
    ImGui::SetNextWindowPos({ds.x * 0.5f, ds.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowSizeConstraints({POP_W, 0.f}, {POP_W, ds.y - 32.f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 16.f, 14.f });

    const auto title = std::string(tr("confirmation")) + "###confirm_copy";
    if (!ImGui::IsPopupOpen("###confirm_copy")) ImGui::OpenPopup("###confirm_copy");
    if (!ImGui::BeginPopupModal(title.c_str(), &g_confirm_copy_open,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
        if (!g_confirm_copy_open) cancel_copy_preview();
        ImGui::PopStyleVar(2);
        return;
    }

    ImGui::TextColored({ 0.90f, 0.75f, 0.25f, 1.f }, "%s", tr("confirmation"));
    ImGui::Spacing();
    ImGui::TextWrapped(
        "%s", tr("copy_warning"));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextDisabled("%s", tr("from"));
    ImGui::TextWrapped("%s", g_confirm_src_label.c_str());
    ImGui::TextDisabled("%s", tr("to"));
    ImGui::TextWrapped("%s", g_confirm_dst_label.c_str());

    ImGui::Spacing();
    ImGui::Separator();
    if (file_tasks_busy()) ImGui::TextUnformatted(tr("working"));
    else if (!g_copy_preview_ready) ImGui::TextWrapped("%s", tr("preview_failed"));
    else {
        ImGui::TextUnformatted(tr("transfer_preview"));
        ImGui::Text("%s: %zu | %s: %llu", tr("affected_files"), g_copy_preview.affected_files,
            tr("transfer_bytes"), static_cast<unsigned long long>(g_copy_preview.changes ? g_copy_preview.copy_bytes : 0));
        if (!g_copy_preview.changes) ImGui::TextWrapped("%s", tr("config_identical"));
        ImGui::BeginChild("##preview_files", {0, 170}, ImGuiChildFlags_Borders);
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(g_copy_preview.entries.size()));
        while (clipper.Step()) for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
            const auto& entry = g_copy_preview.entries[i];
            const char* key = entry.change == ConfigChange::Add ? "preview_add" :
                entry.change == ConfigChange::Replace ? "preview_replace" :
                entry.change == ConfigChange::Remove ? "preview_remove" : "preview_unchanged";
            ImGui::Text("%s | %s%s", tr(key), entry.relative.generic_u8string().c_str(), entry.directory ? "/" : "");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", entry.relative.generic_u8string().c_str());
        }
        ImGui::EndChild();
    }
    ImGui::Spacing();

    if (ImGui::Button(tr("cancel"), { 120, 0 })) {
        g_confirm_copy_open = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, { 0.75f, 0.20f, 0.20f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.85f, 0.25f, 0.25f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.60f, 0.14f, 0.14f, 1.00f });
    ImGui::BeginDisabled(!g_copy_preview_ready || g_scanning.load() || file_tasks_busy() || updates_phase() == UpdatePhase::Installing);
    if (ImGui::Button(tr(g_copy_preview_ready && !g_copy_preview.changes ? "ok" : "replace"), { -1, 0 })) {
        copy_config(&g_copy_preview);
        g_confirm_copy_open = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();
    ImGui::PopStyleColor(3);

    ImGui::EndPopup();
    if (!g_confirm_copy_open) cancel_copy_preview();
    ImGui::PopStyleVar(2);
}

