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
    ImGui::SetNextWindowSize({ 520.f, 0.f }, ImGuiCond_Appearing);
    bool open = update.popup;
    std::string title = std::string(tr("updates")) + id;
    if (ImGui::BeginPopupModal(title.c_str(), &open, ImGuiWindowFlags_AlwaysAutoResize)) {
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
        if (update.phase == UpdatePhase::Downloaded) {
            ImGui::TextWrapped("%s", tr("update_downloaded"));
            ImGui::TextWrapped("%s", update.downloaded_path.c_str());
            if (ImGui::Button(tr("update_open_folder")))
                open_external(std::filesystem::u8path(update.downloaded_path).parent_path().u8string());
        }
        if (update.phase == UpdatePhase::Failed)
            ImGui::TextWrapped("%s", tr_value(update.error_key.c_str(), update.detail).c_str());
        ImGui::Spacing();
        if (update.phase != UpdatePhase::Checking && update.phase != UpdatePhase::Downloading) {
            if (!update.release.download_url.empty() && update.phase != UpdatePhase::Current && update.phase != UpdatePhase::Downloaded) {
                if (ImGui::Button(tr("update_download"))) updates_download();
                ImGui::SameLine();
            }
            if (ImGui::Button(tr("update_release"))) open_external(RELEASE_URL);
            ImGui::SameLine();
            if (ImGui::Button(tr("update_retry"))) updates_check(true);
        }
        ImGui::Spacing();
        if (ImGui::Button(tr("update_close"))) { open = false; ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    if (update.popup && !open) updates_dismiss();
}
