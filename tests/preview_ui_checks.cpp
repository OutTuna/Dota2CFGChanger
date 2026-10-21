#include "ui/ui_internal.h"
#include "app.h"
#include "localization.h"
#include <imgui_internal.h>
#include <cassert>
#include <cmath>
#include <iostream>

AppTheme g_current_theme = AppTheme::Dark;
ImTextureID g_bg_crimson_tex = 0;
ImVec4 g_crimson_child_bg, g_crimson_selected_bg, g_crimson_hovered_bg, g_crimson_text;
ThemeData g_theme_data[5];
void ui_get_theme_palette(AppTheme, ImVec4&, ImVec4&, ImVec4&, ImVec4&) {}
void ensure_theme_metadata_loaded() {}
const char* ui_theme_background_image(AppTheme) { return nullptr; }
bool ui_theme_has_palette_editor(AppTheme) { return false; }
void ui_apply_theme(AppTheme) {}
void ui_open_backups() {}
int avatar_requests = 0;
void avatar_flush_pending() {}
ImTextureID avatar_get(const std::string&) { ++avatar_requests; return 0; }

int main() {
    for (const char* language : {"en", "ru", "uk"}) {
        set_language(language);
        ImGui::CreateContext();
        auto& io = ImGui::GetIO(); io.DisplaySize = {720, 600}; io.DeltaTime = 1.f / 60.f; io.IniFilename = nullptr;
        io.Fonts->AddFontDefault(); unsigned char* pixels; int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        g_copy_preview = {};
        g_copy_preview_ready = true; g_copy_preview.changes = true;
        for (int i = 0; i < 10000; ++i)
            g_copy_preview.entries.push_back({std::filesystem::u8path("remote/cfg/config-" + std::to_string(i) + ".vcfg"), ConfigChange::Replace, false, 42});
        g_confirm_src_label = "Source (42)"; g_confirm_dst_label = "Destination (43)";
        g_confirm_copy_open = true;
        src_list.clear(); dst_list.clear();
        for (int i = 0; i < 10000; ++i) { src_list.push_back(std::to_string(i + 1)); dst_list.push_back(std::to_string(i + 1)); }
        app_ui_changed(); avatar_requests = 0;
        for (int frame = 0; frame < 5; ++frame) {
            ImGui::NewFrame(); ui_render_main(nullptr, io.DisplaySize); ImGui::Render();
        }
        assert(avatar_requests < 200);
        const auto title = std::string(tr("confirmation")) + "###confirm_copy";
        auto* window = ImGui::FindWindowByName(title.c_str());
        assert(window && window->Active && (window->Flags & ImGuiWindowFlags_Modal));
        assert(window->Size.x <= io.DisplaySize.x && window->Size.y <= io.DisplaySize.y);
        assert(std::abs(window->Pos.x + window->Size.x / 2 - io.DisplaySize.x / 2) < 2);

        assert(ImGui::GetDrawData()->TotalVtxCount < 10000);
        io.AddKeyEvent(ImGuiKey_Escape, true);
        ImGui::NewFrame(); ui_render_main(nullptr, io.DisplaySize); ImGui::Render();
        assert(!g_confirm_copy_open);
        assert(!g_copy_preview_ready);
        ImGui::DestroyContext();
    }
    std::cout << "Account lists and transfer preview fit EN/RU/UA and clip 10000 entries\n";
}
