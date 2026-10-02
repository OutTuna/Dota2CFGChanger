#include "ui_internal.h"
#include "app.h"
#include "localization.h"
#include "updates.h"
#include "avatar.h"

void ui_render_main(ImFont* font_big, ImVec2 ds) {
    avatar_flush_pending();

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(ds);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {16.f, 14.f});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {8.f, 6.f});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8.f, 6.f});
    ImGui::Begin("Main", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

    if (ui_theme_background_image(g_current_theme) && g_bg_crimson_tex != (ImTextureID)0) {
        ImVec2 wpos = ImGui::GetWindowPos();
        ImGui::GetWindowDrawList()->AddImage(
            g_bg_crimson_tex,
            wpos,
            { wpos.x + ds.x, wpos.y + ds.y },
            { 0, 0 }, { 1, 1 },
            IM_COL32(255, 255, 255, 255));
        ImGui::GetWindowDrawList()->AddRectFilled(wpos,
            { wpos.x + ds.x, wpos.y + ds.y }, IM_COL32(12, 14, 18, 205));
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

    ensure_theme_metadata_loaded();
    const float header_y = ImGui::GetStyle().WindowPadding.y + 6.f;
    const float theme_width = 174.f;
    const float language_width = ImGui::CalcTextSize("RU").x
        + ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.f + 8.f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float palette_width = ui_theme_has_palette_editor(g_current_theme) ? 38.f + spacing : 0.f;
    ImGui::SameLine();
    ImGui::SetCursorPos({ ds.x - ImGui::GetStyle().WindowPadding.x
        - theme_width - language_width - spacing - palette_width, header_y });
    ImGui::BeginChild("##header_controls",
        {theme_width + language_width + spacing + palette_width, 36.f}, false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
    if (palette_width > 0.f) {
        if (ImGui::Button("...##palette", { 38.f, ImGui::GetFrameHeight() }))
            g_palette_open = !g_palette_open;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tr("palette"));
        ImGui::SameLine();
    }
    const char* language_labels[] = {"RU", "UA", "EN"};
    const char* language_codes[] = {"ru", "uk", "en"};
    int selected_language = language_index() == 1 ? 0 : language_index() == 2 ? 1 : 2;
    ImGui::SetNextItemWidth(language_width);
    if (ImGui::Combo("##language", &selected_language, language_labels, 3)) {
        set_language(language_codes[selected_language]);
        save_settings();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tr("language"));
    ImGui::SameLine();
    const int theme_index = static_cast<int>(g_current_theme);
    const char* theme_preview = theme_index == 0 ? "Dark" : g_theme_data[theme_index].name.c_str();
    ImGui::SetNextItemWidth(theme_width);
    if (ImGui::BeginCombo("##theme", theme_preview)) {
        for (int i = 0; i < 5; ++i) {
            const char* label = i == 0 ? "Dark" : g_theme_data[i].name.c_str();
            const bool selected = i == theme_index;
            if (ImGui::Selectable(label, selected)) {
                ui_apply_theme(static_cast<AppTheme>(i));
                save_settings();
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tr("theme"));
    ImGui::EndChild();
    ImGui::SetCursorPosY(ImGui::GetStyle().WindowPadding.y + 44.f);

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
    ImGui::InputText("##dst_path", dst_path, PATH_BUF_SIZE, ImGuiInputTextFlags_ReadOnly);
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

    const float bottom_h = 38.f + 40.f + 32.f + ImGui::GetStyle().ItemSpacing.y
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
    const float footer_y = ds.y - ImGui::GetStyle().WindowPadding.y - 32.f;
    ImGui::SetCursorPosY(footer_y - 32.f - ImGui::GetStyle().ItemSpacing.y);
    const auto status_text = tr_value("status", local_status);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {10.f, 7.f});
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 7.f);
    ImGui::BeginChild("##status", {0, 32.f},
        ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::TextUnformatted(status_text.c_str());
    ImGui::EndChild();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", status_text.c_str());
    ImGui::PopStyleVar(2);
    ImGui::SetCursorPosY(footer_y);
    if (ImGui::Button("Info", {64.f, 32.f})) ImGui::OpenPopup("Info###info");
    ImGui::SetNextWindowPos({ds.x * 0.5f, ds.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowSizeConstraints({280.f, 0.f}, {280.f, ds.y - 32.f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {20.f, 18.f});
    bool info_open = true;
    if (ImGui::BeginPopupModal("Info###info", &info_open,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        ImGui::TextUnformatted("OutTuna");
        ImGui::TextDisabled("Release v%s", app_version());
        ImGui::Spacing();
        if (ImGui::Button("GitHub", {-1.f, 32.f})) open_external(REPOSITORY_URL);
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    ImGui::SameLine();
    if (ImGui::Button(tr("update_check"), {0.f, 32.f})) updates_check(true);
    ImGui::SameLine();
    ImGui::BeginDisabled(g_scanning.load() || selected_dst.load() < 0);
    if (ImGui::Button(tr("backups"), {0.f, 32.f})) ui_open_backups();
    ImGui::EndDisabled();

    ImGui::End();
    ImGui::PopStyleVar(3);

    if (g_palette_open && ui_theme_has_palette_editor(g_current_theme))
        render_palette_panel(ds);

    if (g_confirm_copy_open)
        render_confirm_copy_popup(ds);
}

