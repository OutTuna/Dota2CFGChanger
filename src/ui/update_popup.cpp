#include "ui.h"
#include "updates.h"
#include "platform.h"
#include "localization.h"
#include <filesystem>
#include <algorithm>

void ui_render_update_popup() {
    auto update = updates_snapshot();
    const char* id = "###update_popup";
    if (update.popup && !ImGui::IsPopupOpen(id)) ImGui::OpenPopup(id);
    const auto display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({display.x * 0.5f, display.y * 0.5f},
        ImGuiCond_Always, {0.5f, 0.5f});
    const float width = std::min(360.f, display.x - 32.f);
    ImGui::SetNextWindowSizeConstraints({width, 0.f}, {width, display.y - 32.f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {20.f, 18.f});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8.f, 10.f});
    bool open = update.popup;
    std::string title = std::string(tr("updates")) + id;
    if (ImGui::BeginPopupModal(title.c_str(), &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        float button_width = 170.f;
        for (const char* key : {"update_release", "update_retry", "update_close", "update_download", "update_open_folder", "update_install"})
            button_width = std::max(button_width, ImGui::CalcTextSize(tr(key)).x + 24.f);
        button_width = std::min(button_width, ImGui::GetContentRegionAvail().x);
        auto button = [&](const char* key) {
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - button_width) * 0.5f);
            return ImGui::Button(tr(key), {button_width, 34.f});
        };
        ImGui::Text("%s: v%s", tr("current_version"), app_version());
        ImGui::Spacing();
        if (update.phase == UpdatePhase::Checking) ImGui::TextUnformatted(tr("update_checking"));
        if (update.phase == UpdatePhase::Current) ImGui::TextUnformatted(tr("update_current"));
        if (update.phase == UpdatePhase::Available || update.phase == UpdatePhase::Downloading) {
            ImGui::TextWrapped("%s", tr_value("update_available", update.release.version).c_str());
        }
        if (update.phase == UpdatePhase::Downloading) {
            ImGui::ProgressBar(std::clamp(update.progress, 0.f, 1.f), { -1, 0 });
            ImGui::TextUnformatted(tr("update_downloading"));
        }
        if (update.phase == UpdatePhase::Installing) ImGui::TextWrapped("%s", tr("update_installing"));
        if (update.phase == UpdatePhase::Downloaded) {
            ImGui::TextWrapped("%s", tr("update_downloaded"));
            ImGui::TextWrapped("%s", update.downloaded_path.c_str());
            if (button("update_open_folder"))
                open_external(std::filesystem::u8path(update.downloaded_path).parent_path().u8string());
        }
        if (update.phase == UpdatePhase::Failed)
            ImGui::TextWrapped("%s", tr_value(update.error_key.c_str(), update.detail).c_str());
        ImGui::Spacing();
        if (!updates_self_install_enabled()) ImGui::TextWrapped("%s", tr("update_package_managed"));
        if (update.phase != UpdatePhase::Checking && update.phase != UpdatePhase::Downloading && update.phase != UpdatePhase::Installing) {
            if (updates_self_install_enabled() && update.phase == UpdatePhase::Failed && !update.downloaded_path.empty()) {
                if (button("update_install")) updates_install();
            }
            if (updates_self_install_enabled() && !update.release.download_url.empty() && update.phase != UpdatePhase::Current && update.phase != UpdatePhase::Downloaded) {
                if (button("update_download")) updates_download();
            }
            if (button("update_release")) open_external(RELEASE_URL);
            if (button("update_retry")) updates_check(true);
        }
        ImGui::Spacing();
        if (button("update_close")) { open = false; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    if (update.popup && !open) updates_dismiss();
}
