#include "ui.h"
#include "app_state.h"
#include "app_logic.h"
#include "backends/imgui_impl_glfw.h"
#include <GLFW/glfw3.h>
#include <thread>
#include <cmath>

void ui_apply_theme() {
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding    = 8.0f;
    st.ChildRounding     = 6.0f;
    st.FrameRounding     = 5.0f;
    st.PopupRounding     = 6.0f;
    st.ScrollbarRounding = 5.0f;
    st.GrabRounding      = 4.0f;
    st.TabRounding       = 5.0f;
    st.WindowBorderSize  = 1.0f;
    st.FrameBorderSize   = 0.0f;
    st.WindowPadding     = {14.0f, 14.0f};
    st.FramePadding      = {10.0f, 6.0f};
    st.ItemSpacing       = {8.0f, 5.0f};
    st.ItemInnerSpacing  = {6.0f, 3.0f};
    st.ScrollbarSize     = 10.0f;
    st.GrabMinSize       = 8.0f;
    st.IndentSpacing     = 18.0f;

    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg]             = {0.09f, 0.09f, 0.10f, 1.00f};
    c[ImGuiCol_ChildBg]              = {0.11f, 0.11f, 0.13f, 1.00f};
    c[ImGuiCol_PopupBg]              = {0.10f, 0.10f, 0.12f, 1.00f};
    c[ImGuiCol_Border]               = {0.22f, 0.22f, 0.26f, 1.00f};
    c[ImGuiCol_BorderShadow]         = {0.00f, 0.00f, 0.00f, 0.00f};
    c[ImGuiCol_FrameBg]              = {0.15f, 0.15f, 0.18f, 1.00f};
    c[ImGuiCol_FrameBgHovered]       = {0.20f, 0.20f, 0.24f, 1.00f};
    c[ImGuiCol_FrameBgActive]        = {0.24f, 0.24f, 0.28f, 1.00f};
    c[ImGuiCol_TitleBg]              = {0.07f, 0.07f, 0.08f, 1.00f};
    c[ImGuiCol_TitleBgActive]        = {0.07f, 0.07f, 0.08f, 1.00f};
    c[ImGuiCol_TitleBgCollapsed]     = {0.07f, 0.07f, 0.08f, 1.00f};
    c[ImGuiCol_ScrollbarBg]          = {0.09f, 0.09f, 0.10f, 1.00f};
    c[ImGuiCol_ScrollbarGrab]        = {0.28f, 0.28f, 0.32f, 1.00f};
    c[ImGuiCol_ScrollbarGrabHovered] = {0.38f, 0.38f, 0.44f, 1.00f};
    c[ImGuiCol_ScrollbarGrabActive]  = {0.50f, 0.50f, 0.58f, 1.00f};
    c[ImGuiCol_CheckMark]            = {0.90f, 0.25f, 0.25f, 1.00f};
    c[ImGuiCol_SliderGrab]           = {0.80f, 0.22f, 0.22f, 1.00f};
    c[ImGuiCol_SliderGrabActive]     = {1.00f, 0.30f, 0.30f, 1.00f};
    c[ImGuiCol_Button]               = {0.20f, 0.20f, 0.24f, 1.00f};
    c[ImGuiCol_ButtonHovered]        = {0.75f, 0.20f, 0.20f, 1.00f};
    c[ImGuiCol_ButtonActive]         = {0.55f, 0.14f, 0.14f, 1.00f};
    c[ImGuiCol_Header]               = {0.75f, 0.20f, 0.20f, 0.40f};
    c[ImGuiCol_HeaderHovered]        = {0.75f, 0.20f, 0.20f, 0.65f};
    c[ImGuiCol_HeaderActive]         = {0.75f, 0.20f, 0.20f, 0.90f};
    c[ImGuiCol_Separator]            = {0.22f, 0.22f, 0.26f, 1.00f};
    c[ImGuiCol_SeparatorHovered]     = {0.75f, 0.20f, 0.20f, 0.70f};
    c[ImGuiCol_SeparatorActive]      = {0.75f, 0.20f, 0.20f, 1.00f};
    c[ImGuiCol_ResizeGrip]           = {0.75f, 0.20f, 0.20f, 0.20f};
    c[ImGuiCol_ResizeGripHovered]    = {0.75f, 0.20f, 0.20f, 0.60f};
    c[ImGuiCol_ResizeGripActive]     = {0.75f, 0.20f, 0.20f, 0.90f};
    c[ImGuiCol_Tab]                  = {0.15f, 0.15f, 0.18f, 1.00f};
    c[ImGuiCol_TabHovered]           = {0.75f, 0.20f, 0.20f, 0.80f};
    c[ImGuiCol_TabActive]            = {0.60f, 0.16f, 0.16f, 1.00f};
    c[ImGuiCol_TabUnfocused]         = {0.12f, 0.12f, 0.14f, 1.00f};
    c[ImGuiCol_TabUnfocusedActive]   = {0.30f, 0.14f, 0.14f, 1.00f};
    c[ImGuiCol_TextSelectedBg]       = {0.75f, 0.20f, 0.20f, 0.35f};
    c[ImGuiCol_NavHighlight]         = {0.75f, 0.20f, 0.20f, 1.00f};
    c[ImGuiCol_Text]                 = {0.92f, 0.92f, 0.94f, 1.00f};
    c[ImGuiCol_TextDisabled]         = {0.45f, 0.45f, 0.50f, 1.00f};
}

void ui_render_main(ImFont* font_big, ImVec2 ds) {
    (void)font_big;

    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(ds);
    ImGui::Begin("Main", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

    ImGui::TextColored({1,0,0,1}, "DOTA 2 C++ MANAGER");
    ImGui::SameLine();
    ImGui::TextDisabled("  by OutTuna");
    ImGui::Separator();

    static bool open_src = false;
    static bool open_dst = false;

    ImGui::TextDisabled("Откуда конфиг");
    ImGui::PushItemWidth(-1);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, {0.28f, 0.28f, 0.32f, 1.0f});
    ImGui::InputText("##src_path", src_path, 256,
        ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();
    if (ImGui::IsItemClicked()) open_src = true;
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::TextDisabled("Куда конфиг");
    ImGui::PushItemWidth(-1);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, {0.28f, 0.28f, 0.32f, 1.0f});
    ImGui::InputText("##dst_path", dst_path, 256,
        ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();
    if (ImGui::IsItemClicked()) open_dst = true;
    ImGui::PopItemWidth();
    ImGui::Spacing();

#ifdef _WIN32
    if (open_src) {
        open_src = false;
        auto p = browse_for_folder("Откуда конфиг");
        if (!p.empty()) { strncpy(src_path, p.c_str(), 255); src_path[255] = 0; }
    }
    if (open_dst) {
        open_dst = false;
        auto p = browse_for_folder("Куда конфиг");
        if (!p.empty()) { strncpy(dst_path, p.c_str(), 255); dst_path[255] = 0; }
    }
#endif

    if (ImGui::Button("SCAN FOLDERS", {-1, 32}))
        std::thread(scan_thread).detach();

    ImGui::Columns(2, "lists", true);
    ImGui::Text("From (Config)");

    auto getter = [](void* data, int idx, const char** out) -> bool {
        auto& v = *static_cast<std::vector<std::string>*>(data);
        if (idx < 0 || idx >= (int)v.size()) return false;
        static std::string buf;
        const std::string& id = v[idx];
        buf = nick_cache.count(id) ? nick_cache[id] + " (" + id + ")" : id;
        *out = buf.c_str();
        return true;
    };

    ImGui::ListBox("##src", &selected_src, getter, &src_list, (int)src_list.size(), 8);
    ImGui::NextColumn();
    ImGui::Text("To (Account)");
    ImGui::ListBox("##dst", &selected_dst, getter, &dst_list, (int)dst_list.size(), 8);
    ImGui::Columns(1);

    ImGui::Separator();
    if (ImGui::Button("COPY CONFIG NOW", {-1, 38})) copy_config();
    ImGui::Text("Status: %s", status_msg.c_str());

    ImGui::End();
}

void ui_render_success_popup(ImFont* font_big, ImVec2 ds, float delta_time) {
    if (!g_success.show) return;

    const float OPEN_DUR  = 0.30f;
    const float CLOSE_DUR = 0.25f;

    if (!g_success.closing) {
        g_success.anim_t += delta_time;
    } else {
        g_success.close_t += delta_time;
        if (g_success.close_t >= CLOSE_DUR) { g_success.show = false; return; }
    }

    float t_open    = (g_success.anim_t / OPEN_DUR < 1.0f) ? g_success.anim_t / OPEN_DUR : 1.0f;
    float ease_open = t_open * t_open * (3.0f - 2.0f * t_open);

    float t_close   = g_success.closing ? (g_success.close_t / CLOSE_DUR) : 0.0f;
    if (t_close > 1.0f) t_close = 1.0f;
    float ease_close = t_close * t_close;

    float scale = (0.82f + 0.18f * ease_open) * (1.0f - 0.15f * ease_close);
    float alpha = ease_open * (1.0f - ease_close);
    float fly_y = (1.0f - ease_open) * 30.0f + ease_close * (-20.0f);

    const float POP_W    = 460.0f;
    const float POP_H    = 300.0f;
    const float ROUNDING = 14.0f;
    const float cx       = ds.x * 0.5f;
    const float cy       = ds.y * 0.5f + fly_y;
    const float sw       = POP_W * scale;
    const float sh       = POP_H * scale;
    const float pop_x    = cx - sw * 0.5f;
    const float pop_y    = cy - sh * 0.5f;
    const float pop_x2   = cx + sw * 0.5f;
    const float pop_y2   = cy + sh * 0.5f;
    const float time     = (float)glfwGetTime();
    const float pulse    = 0.5f + 0.5f * sinf(time * 2.8f);

    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(ds);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, {0,0,0,0});
    ImGui::Begin("##dim", NULL,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoInputs      | ImGuiWindowFlags_NoNav);

    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled({0,0}, ds, IM_COL32(0,0,0,(int)(150*alpha)));

    struct GL { float p; int a; };
    GL glows[] = {{10,60},{22,40},{38,24},{58,12},{82,6}};
    for (auto& g : glows)
        dl->AddRectFilled({pop_x-g.p, pop_y-g.p}, {pop_x2+g.p, pop_y2+g.p},
            IM_COL32(30,200,80,(int)(g.a*alpha)), ROUNDING + g.p*0.55f);

    dl->AddRectFilled({pop_x,pop_y},{pop_x2,pop_y2},
        IM_COL32(16,17,20,(int)(255*alpha)), ROUNDING);

    float bar_h = 3.5f;
    float bar_pulse = 0.75f + 0.25f * sinf(time * 2.0f);
    dl->AddRectFilled({pop_x, pop_y}, {pop_x2, pop_y + bar_h},
        IM_COL32(35, 185, 80, (int)(255 * bar_pulse * alpha)),
        ROUNDING, ImDrawFlags_RoundCornersTop);

    dl->AddRect({pop_x,pop_y},{pop_x2,pop_y2},
        IM_COL32(35,185,80,(int)((100+60*pulse)*alpha)), ROUNDING, 0, 1.0f);

    ImGui::End();
    ImGui::PopStyleColor();

    ImGui::SetNextWindowPos({pop_x, pop_y});
    ImGui::SetNextWindowSize({sw, sh});
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, {0,0,0,0});
    ImGui::PushStyleColor(ImGuiCol_Border,   {0,0,0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   ROUNDING);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    {20.0f*scale, 14.0f*scale});
    ImGui::Begin("##success_popup", NULL,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove        | ImGuiWindowFlags_NoScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

    if (font_big) ImGui::PushFont(font_big);
    {
        const char* title = "SUCCESS";
        float tw = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((sw - tw) * 0.5f);
        float gv = 0.72f + 0.28f * sinf(time * 2.2f);
        ImGui::TextColored({0.10f, gv, 0.35f, 1.0f}, "%s", title);
    }
    if (font_big) ImGui::PopFont();

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, {0.15f,0.70f,0.30f,0.25f});
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    auto row = [](const char* lbl, ImVec4 lc, const char* val, ImVec4 vc) {
        ImGui::TextColored(lc, "%s", lbl);
        ImGui::SameLine();
        ImGui::TextColored(vc, "%s", val);
    };

    std::string src_d = g_success.src_nick + "  (ID: " + g_success.src_id + ")";
    std::string dst_d = g_success.dst_nick + "  (ID: " + g_success.dst_id + ")";
    row("Откуда: ", {0.55f,0.55f,0.60f,1}, src_d.c_str(), {0.92f,0.92f,0.94f,1});
    row("Куда:   ", {0.55f,0.55f,0.60f,1}, dst_d.c_str(), {0.92f,0.92f,0.94f,1});

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, {0.55f,0.55f,0.60f,0.20f});
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::TextColored({0.55f,0.55f,0.60f,1}, "Source:");
    ImGui::SameLine();
    ImGui::TextColored({0.55f,0.55f,0.60f,1}, "%s", g_success.src_folder.c_str());
    ImGui::TextColored({0.55f,0.55f,0.60f,1}, "Dest:  ");
    ImGui::SameLine();
    ImGui::TextColored({0.55f,0.55f,0.60f,1}, "%s", g_success.dst_folder.c_str());

    ImGui::Spacing(); ImGui::Spacing();

    float btn_gv = 0.55f + 0.12f*pulse;
    const float btn_w = 120.0f*scale, btn_h = 32.0f*scale;
    ImGui::SetCursorPosX((sw - btn_w) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Button,        {0.10f, btn_gv, 0.20f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.14f, 0.78f, 0.28f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.07f, 0.42f, 0.15f, 1.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    if (ImGui::Button("OK", {btn_w, btn_h})) {
        g_success.closing = true;
        g_success.close_t = 0.0f;
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::PopStyleVar();
    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}