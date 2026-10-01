#include "ui_internal.h"
#include "app.h"
#include "localization.h"
#include "updates.h"
#include "avatar.h"
#include <GLFW/glfw3.h>

void ui_render_main(ImFont* font_big, ImVec2 ds) {
    avatar_flush_pending();

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(ds);
    ImGui::Begin("Main", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

    if (ui_theme_background_image(g_current_theme) && g_bg_crimson_tex != (ImTextureID)0) {
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
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float th = ImGui::CalcTextSize(title).y;

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

    if (ui_theme_has_palette_editor(g_current_theme)) {
        ImGui::SameLine(ds.x - 70.f);
        ImGui::PushStyleColor(ImGuiCol_Button, { 0.00f, 0.00f, 0.00f, 0.00f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.40f, 0.08f, 0.08f, 1.00f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.25f, 0.05f, 0.05f, 1.00f });
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);
        if (ImGui::Button("##palette", { 26, 22 }))
            g_palette_open = !g_palette_open;
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImDrawList* pdl = ImGui::GetWindowDrawList();
        ImVec2 pmin = ImGui::GetItemRectMin();
        ImVec2 pmax = ImGui::GetItemRectMax();
        float pcx = (pmin.x + pmax.x) * 0.5f;
        float pcy = (pmin.y + pmax.y) * 0.5f;
        float pr = 4.5f;
        ImVec2 dots[4] = {
            { pcx - pr, pcy - pr }, { pcx + pr, pcy - pr },
            { pcx - pr, pcy + pr }, { pcx + pr, pcy + pr }
        };
        ImU32 dot_cols[4] = {
            IM_COL32(220, 60, 60, 220), IM_COL32(60, 120, 220, 220),
            IM_COL32(60, 200, 80, 220), IM_COL32(220, 180, 40, 220)
        };
        for (int i = 0; i < 4; i++)
            pdl->AddCircleFilled(dots[i], 3.f, dot_cols[i]);
    }

    ImGui::SameLine(ds.x - 38.f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);
    ImGui::PushStyleColor(ImGuiCol_Button, { 0.00f, 0.00f, 0.00f, 0.00f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.25f, 0.25f, 0.28f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.18f, 0.18f, 0.20f, 1.00f });
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);
    if (ImGui::Button("##gear", { 26, 22 }))
        g_settings_open = !g_settings_open;
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 bmin = ImGui::GetItemRectMin();
    ImVec2 bmax = ImGui::GetItemRectMax();
    ImVec2 ctr = { (bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f };

    float t = (float)glfwGetTime();
    float ang = g_settings_open ? t * 1.2f : 0.f;
    ImU32 col = IM_COL32(160, 160, 170, 220);
    const float PI = 3.14159f;

    for (int i = 0; i < 8; i++) {
        float a0 = ang + i * (PI * 2.f / 8.f);
        float a1 = a0 + 0.35f;
        dl->PathArcTo(ctr, 6.5f, a0, a1, 4);
        dl->PathArcTo(ctr, 3.5f, a1, a0 + PI * 2.f / 8.f, 4);
        dl->PathFillConvex(col);
    }
    dl->AddCircleFilled(ctr, 2.8f, col);

    ImGui::Separator();

    bool scanning = g_scanning.load();
    if (scanning) ImGui::BeginDisabled();
    static bool open_src = false;
    static bool open_dst = false;

    ImGui::TextDisabled("%s", tr("source"));
    ImGui::PushItemWidth(-1);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, { 0.28f, 0.28f, 0.32f, 1.f });
    ImGui::InputText("##src_path", src_path, PATH_BUF_SIZE, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();
    if (ImGui::IsItemClicked()) open_src = true;
    ImGui::PopItemWidth();

    ImGui::Spacing();

    ImGui::TextDisabled("%s", tr("destination"));
    ImGui::PushItemWidth(-1);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, { 0.28f, 0.28f, 0.32f, 1.f });
    ImGui::InputText("##dst_path", dst_path, 256, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();
    if (ImGui::IsItemClicked()) open_dst = true;
    ImGui::PopItemWidth();

    ImGui::Spacing();

    if (open_src) {
        open_src = false;
        auto p = browse_for_folder(tr("source"));
        if (!p.empty()) { set_config_path(src_path, p); save_settings(); }
    }
    if (open_dst) {
        open_dst = false;
        auto p = browse_for_folder(tr("destination"));
        if (!p.empty()) { set_config_path(dst_path, p); save_settings(); }
    }

    if (scanning) ImGui::EndDisabled();
    if (scanning) ImGui::BeginDisabled();
    if (ImGui::Button(scanning ? tr("scanning") : tr("scan"), { -1, 32 }))
        start_scan();
    if (scanning) ImGui::EndDisabled();

    const float AVATAR_SIZE = 24.f;
    const float ROW_H = AVATAR_SIZE + 6.f;

    const float bottom_h = 38.f + 40.f + ImGui::GetFrameHeightWithSpacing()
        + ImGui::GetStyle().ItemSpacing.y * 3
        + ImGui::GetStyle().WindowPadding.y;
    const float list_h = ImGui::GetContentRegionAvail().y - bottom_h;

    std::vector<std::string> local_src_list, local_dst_list;
    std::map<std::string, std::string> local_nick_cache;
    std::string local_status;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        local_src_list = src_list;
        local_dst_list = dst_list;
        local_nick_cache = nick_cache;
        local_status = tr_value(status_msg.c_str(), status_detail);
    }

    ImGui::Columns(2, "lists", true);
    ImGui::TextUnformatted(tr("from_account"));

    auto render_account_list = [&](
        const std::vector<std::string>& list,
        std::atomic<int>& selected_atomic,
        const char* child_id)
    {
        int selected = selected_atomic.load();
        bool use_custom_palette = ui_theme_has_palette_editor(g_current_theme);

        ImGui::BeginChild(child_id, { 0, list_h }, true);

        for (int i = 0; i < (int)list.size(); i++) {
            const std::string& id = list[i];
            std::string nick = local_nick_cache.count(id) ? local_nick_cache[id] : id;
            std::string label = nick + " (" + id + ")";
            bool sel = (i == selected);

            ImGui::PushID(i);

            ImVec2 row_pos = ImGui::GetCursorScreenPos();

            if (use_custom_palette) {
                ImVec4 row_col = sel ? g_crimson_selected_bg : g_crimson_child_bg;
                ImGui::GetWindowDrawList()->AddRectFilled(
                    row_pos,
                    { row_pos.x + ImGui::GetContentRegionAvail().x, row_pos.y + ROW_H },
                    ImGui::ColorConvertFloat4ToU32(row_col));
            }

            if (use_custom_palette) {
                ImGui::PushStyleColor(ImGuiCol_Header, g_crimson_selected_bg);
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, g_crimson_hovered_bg);
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, g_crimson_selected_bg);
            }

            if (ImGui::Selectable("##row", sel,
                ImGuiSelectableFlags_None, { 0, ROW_H })) {
                selected = i;
                selected_atomic.store(i);
            }

            if (use_custom_palette) ImGui::PopStyleColor(3);

            ImDrawList* dl = ImGui::GetWindowDrawList();
            const float pad = 5.f;
            const float radius = AVATAR_SIZE * 0.5f;
            ImVec2 av_ctr = {
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
            if (use_custom_palette) {
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

    render_account_list(local_src_list, selected_src, "##src_list");

    ImGui::NextColumn();
    ImGui::TextUnformatted(tr("to_account"));
    render_account_list(local_dst_list, selected_dst, "##dst_list");

    ImGui::Columns(1);
    ImGui::Separator();

    if (scanning) ImGui::BeginDisabled();
    if (ImGui::Button(tr("copy"), { -1, 38 })) {
        int s = selected_src.load(), d = selected_dst.load();
        if (s < 0 || d < 0) {
            copy_config();
        } else if (s < (int)local_src_list.size() && d < (int)local_dst_list.size()) {
            const std::string& s_id = local_src_list[s];
            const std::string& d_id = local_dst_list[d];
            std::string s_nick = local_nick_cache.count(s_id) ? local_nick_cache[s_id] : s_id;
            std::string d_nick = local_nick_cache.count(d_id) ? local_nick_cache[d_id] : d_id;
            g_confirm_src_label = s_nick + " (" + s_id + ")";
            g_confirm_dst_label = d_nick + " (" + d_id + ")";
            g_confirm_copy_open = true;
        }
    }

    if (scanning) ImGui::EndDisabled();
    ImGui::BeginChild("##status", {0, 40.f});
    ImGui::TextWrapped("%s", tr_value("status", local_status).c_str());
    ImGui::EndChild();
    if (ImGui::SmallButton("OutTuna")) open_external(REPOSITORY_URL);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", REPOSITORY_URL);
    ImGui::SameLine();
    if (ImGui::SmallButton(tr("update_check"))) updates_check(true);
    ImGui::SameLine();
    ImGui::TextDisabled("v%s", app_version());

    ImGui::End();

    if (g_settings_open)
        render_settings_panel(ds);

    if (g_palette_open && ui_theme_has_palette_editor(g_current_theme))
        render_palette_panel(ds);

    if (g_confirm_copy_open)
        render_confirm_copy_popup(ds);
}

