#include "ui/ui.h"
#include "updates.h"
#include "localization.h"
#include "platform.h"
#include "app.h"
#include "backups.h"
#include <filesystem>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include <imgui_internal.h>
#include <cassert>
#include <iostream>
#include <cmath>

UpdateSnapshot snapshot;
UpdateSnapshot updates_snapshot() { return snapshot; }
void updates_dismiss() { snapshot.popup = false; }
void updates_download() {}
void updates_install() {}
void updates_check(bool) {}
const char* app_version() { return "1.9"; }

int main() {
    for (const char* language : {"en", "ru", "uk"}) {
        set_language(language);
        for (auto phase : {UpdatePhase::Checking, UpdatePhase::Available, UpdatePhase::Current,
            UpdatePhase::Downloading, UpdatePhase::Downloaded, UpdatePhase::Installing, UpdatePhase::Failed}) {
            ImGui::CreateContext();
            auto& io = ImGui::GetIO();
            io.DisplaySize = {720, 560}; io.DeltaTime = 1.f / 60.f; io.IniFilename = nullptr;
            auto fonts = find_font_paths();
            if (!fonts.regular.empty()) io.Fonts->AddFontFromFileTTF(fonts.regular.c_str(), 16, nullptr, io.Fonts->GetGlyphRangesCyrillic());
            else io.Fonts->AddFontDefault();
            unsigned char* pixels; int w, h;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
            snapshot = {}; snapshot.popup = true; snapshot.phase = phase;
            snapshot.release.version = "v1.10";
            snapshot.release.download_url = "https://github.com/OutTuna/Dota2CFGChanger/releases/download/latest/DotaManager.exe";
            snapshot.progress = 0.5f;
            snapshot.error_key = "update_download_failed"; snapshot.detail = "Connection timed out";
            snapshot.downloaded_path = "C:/Users/user/AppData/Roaming/DotaManager/updates/v1.10/DotaManager.exe";
            for (int frame = 0; frame < 5; ++frame) {
                ImGui::NewFrame(); ui_render_update_popup(); ImGui::Render();
            }
            auto window = ImGui::FindWindowByName("Updates###update_popup");
            assert(window && window->Active);
            assert(window->Size.x <= io.DisplaySize.x && window->Size.y <= io.DisplaySize.y);
            assert(std::abs(window->Pos.x + window->Size.x * 0.5f - io.DisplaySize.x * 0.5f) <= 1.f);
            assert(std::abs(window->Pos.y + window->Size.y * 0.5f - io.DisplaySize.y * 0.5f) <= 1.f);
            ImGui::DestroyContext();
        }
    }
    namespace fs = std::filesystem;
    auto root = fs::temp_directory_path() / ("dotamanager-backup-ui-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
#ifdef _WIN32
    _wputenv_s(L"APPDATA", root.c_str());
#else
    setenv("XDG_CONFIG_HOME", root.c_str(), 1);
#endif
    dst_list = {"123"}; selected_dst = 0;
    auto userdata = root / "userdata";
    auto destination = userdata / "123/570";
    fs::create_directories(destination);
    auto path = userdata.u8string();
    std::copy(path.begin(), path.end(), dst_path); dst_path[path.size()] = 0;
    for (bool populated : {false, true}) {
        if (populated) {
            for (int i = 0; i < 5; ++i) {
                std::ofstream(destination / "config.txt") << i;
                create_config_backup(destination, fs::u8path(config_dir()) / "backups", "123");
            }
        }
        for (const char* language : {"en", "ru", "uk"}) {
            set_language(language);
            ImGui::CreateContext();
            auto& io = ImGui::GetIO(); io.DisplaySize = {720, 600}; io.DeltaTime = 1.f/60.f; io.IniFilename = nullptr;
            auto fonts = find_font_paths();
            if (!fonts.regular.empty()) io.Fonts->AddFontFromFileTTF(fonts.regular.c_str(), 16, nullptr, io.Fonts->GetGlyphRangesCyrillic());
            else io.Fonts->AddFontDefault();
            unsigned char* pixels; int w,h; io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
            ui_open_backups();
            for (int frame=0; frame<5; ++frame) { ImGui::NewFrame(); ui_render_backups_popup(); ImGui::Render(); }
            auto window = ImGui::FindWindowByName("Backups###backups_popup");
            assert(window && window->Active && window->Size.x <= 720 && window->Size.y <= 568);
            assert(std::abs(window->Pos.x + window->Size.x*.5f - 360) <= 1);
            assert(std::abs(window->Pos.y + window->Size.y*.5f - 300) <= 1);
            assert(window->ScrollMax.y == 0);
            ImGui::DestroyContext();
        }
    }
    fs::remove_all(root);
    std::cout << "Update dialog fits all phases in English, Russian and Ukrainian\n";
}
