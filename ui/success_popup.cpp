#include "ui.h"
#include "app.h"
#include <GLFW/glfw3.h>
#include <cmath>

void ui_render_success_popup(ImFont* font_big, ImVec2 ds, float delta_time) {
    if (!g_success.show) return;

    const float OPEN_DUR = 0.30f;
    const float CLOSE_DUR = 0.25f;

    if (!g_success.closing) {
        g_success.anim_t += delta_time;
    } else {
        g_success.close_t += delta_time;
        if (g_success.close_t >= CLOSE_DUR) {
            g_success.show = false;
            return;
        }
    }

    float t_open = (g_success.anim_t / OPEN_DUR < 1.f) ? g_success.anim_t / OPEN_DUR : 1.f;
    float ease_open = t_open * t_open * (3.f - 2.f * t_open);

    float t_close = g_success.closing ? (g_success.close_t / CLOSE_DUR) : 0.f;
    if (t_close > 1.f) t_close = 1.f;
    float ease_close = t_close * t_close;

    float scale = (0.82f + 0.18f * ease_open) * (1.f - 0.15f * ease_close);
    float alpha = ease_open * (1.f - ease_close);
    float fly_y = (1.f - ease_open) * 30.f + ease_close * (-20.f);

    const float POP_W = 460.f;
    const float POP_H = 300.f;
    const float ROUNDING = 14.f;

    const float cx = ds.x * 0.5f;
    const float cy = ds.y * 0.5f + fly_y;
    const float sw = POP_W * scale;
    const float sh = POP_H * scale;
    const float pop_x = cx - sw * 0.5f;
    const float pop_y = cy - sh * 0.5f;
    const float pop_x2 = cx + sw * 0.5f;
    const float pop_y2 = cy + sh * 0.5f;

    const float time = (float)glfwGetTime();
    const float pulse = 0.5f + 0.5f * sinf(time * 2.8f);

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(ds);
    ImGui::SetNextWindowBgAlpha(0.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
    ImGui::Begin("##dim", NULL,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoNav);

    auto* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled({ 0, 0 }, ds, IM_COL32(0, 0, 0, (int)(150 * alpha)));

    struct GlowLayer { float p; int a; };
    GlowLayer glows[] = { { 10, 60 }, { 22, 40 }, { 38, 24 }, { 58, 12 }, { 82, 6 } };
    for (auto& g : glows) {
        dl->AddRectFilled(
            { pop_x - g.p, pop_y - g.p },
            { pop_x2 + g.p, pop_y2 + g.p },
            IM_COL32(30, 200, 80, (int)(g.a * alpha)),
            ROUNDING + g.p * 0.55f);
    }

    dl->AddRectFilled({ pop_x, pop_y }, { pop_x2, pop_y2 },
        IM_COL32(16, 17, 20, (int)(255 * alpha)), ROUNDING);

    float bar_pulse = 0.75f + 0.25f * sinf(time * 2.f);
    dl->AddRectFilled({ pop_x, pop_y }, { pop_x2, pop_y + 3.5f },
        IM_COL32(35, 185, 80, (int)(255 * bar_pulse * alpha)),
        ROUNDING, ImDrawFlags_RoundCornersTop);

    dl->AddRect({ pop_x, pop_y }, { pop_x2, pop_y2 },
        IM_COL32(35, 185, 80, (int)((100 + 60 * pulse) * alpha)),
        ROUNDING, 0, 1.f);

    ImGui::End();
    ImGui::PopStyleColor();

    ImGui::SetNextWindowPos({ pop_x, pop_y });
    ImGui::SetNextWindowSize({ sw, sh });
    ImGui::SetNextWindowBgAlpha(0.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_Border, { 0, 0, 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, ROUNDING);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 20.f * scale, 14.f * scale });

    ImGui::Begin("##success_popup", NULL,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

    if (font_big) ImGui::PushFont(font_big);
    {
        const char* title = "SUCCESS";
        float tw = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((sw - tw) * 0.5f);
        float gv = 0.72f + 0.28f * sinf(time * 2.2f);
        ImGui::TextColored({ 0.10f, gv, 0.35f, 1.f }, "%s", title);
    }
    if (font_big) ImGui::PopFont();

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, { 0.15f, 0.70f, 0.30f, 0.25f });
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    auto row = [](const char* lbl, ImVec4 lc, const char* val, ImVec4 vc) {
        ImGui::TextColored(lc, "%s", lbl);
        ImGui::SameLine();
        ImGui::TextColored(vc, "%s", val);
    };

    std::string src_d = g_success.src_nick + " (ID: " + g_success.src_id + ")";
    std::string dst_d = g_success.dst_nick + " (ID: " + g_success.dst_id + ")";

    row("Откуда: ", { 0.55f, 0.55f, 0.60f, 1 }, src_d.c_str(), { 0.92f, 0.92f, 0.94f, 1 });
    row("Куда: ", { 0.55f, 0.55f, 0.60f, 1 }, dst_d.c_str(), { 0.92f, 0.92f, 0.94f, 1 });

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, { 0.55f, 0.55f, 0.60f, 0.20f });
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::TextColored({ 0.55f, 0.55f, 0.60f, 1 }, "Source:");
    ImGui::SameLine();
    ImGui::TextColored({ 0.55f, 0.55f, 0.60f, 1 }, "%s", g_success.src_folder.c_str());
    ImGui::TextColored({ 0.55f, 0.55f, 0.60f, 1 }, "Dest: ");
    ImGui::SameLine();
    ImGui::TextColored({ 0.55f, 0.55f, 0.60f, 1 }, "%s", g_success.dst_folder.c_str());

    ImGui::Spacing();
    ImGui::Spacing();

    const float btn_w = 120.f * scale;
    const float btn_h = 32.f * scale;
    const float btn_gv = 0.55f + 0.12f * pulse;

    ImGui::SetCursorPosX((sw - btn_w) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Button, { 0.10f, btn_gv, 0.20f, 1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.14f, 0.78f, 0.28f, 1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.07f, 0.42f, 0.15f, 1.f });
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
    if (ImGui::Button("OK", { btn_w, btn_h })) {
        g_success.closing = true;
        g_success.close_t = 0.f;
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::PopStyleVar();
    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}
