#include "ui.h"
#include "app.h"
#include "backups.h"
#include "file_ops.h"
#include "localization.h"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
namespace {
bool requested = false;
bool confirming = false;
std::string account;
fs::path destination;
std::vector<ConfigBackup> backups;
int selected = -1;
std::string message;
fs::path root() { return fs::u8path(config_dir()) / "backups"; }
void refresh() {
    backups = list_config_backups(root(), account);
    auto target = fs::weakly_canonical(destination);
    backups.erase(std::remove_if(backups.begin(), backups.end(), [&](const ConfigBackup& backup) {
        return fs::weakly_canonical(backup.destination) != target;
    }), backups.end());
    selected = -1;
}
}

void ui_open_backups() {
    if (g_scanning.load()) return;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        int index = selected_dst.load();
        if (index < 0 || index >= static_cast<int>(dst_list.size())) return;
        account = dst_list[index];
        destination = fs::u8path(dst_path) / account / DOTA_ID;
    }
    confirming = false;
    message.clear();
    backups.clear();
    try { refresh(); }
    catch (const std::exception& error) { message = tr_value("error", error.what()); }
    requested = true;
}

void ui_render_backups_popup() {
    const char* id = "###backups_popup";
    if (requested) { ImGui::OpenPopup(id); requested = false; }
    auto display = ImGui::GetIO().DisplaySize;
    float width = std::min(500.f, display.x - 32.f);
    ImGui::SetNextWindowPos({display.x * .5f, display.y * .5f}, ImGuiCond_Always, {.5f, .5f});
    ImGui::SetNextWindowSizeConstraints({width, 0.f}, {width, display.y - 32.f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {20.f, 18.f});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8.f, 10.f});
    bool open = true;
    std::string title = std::string(tr("backups")) + id;
    if (ImGui::BeginPopupModal(title.c_str(), &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        ImGui::TextWrapped("%s", tr_value("backup_account", account).c_str());
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("%s", destination.u8string().c_str());
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", destination.u8string().c_str());
        ImGui::Separator();
        if (confirming && selected >= 0 && selected < static_cast<int>(backups.size())) {
            ImGui::TextWrapped("%s", tr_value("backup_restore_warning", backups[selected].date).c_str());
            if (ImGui::Button(tr("cancel"), {-1.f, 34.f})) confirming = false;
            if (ImGui::Button(tr("backup_restore"), {-1.f, 34.f})) {
                try {
                    auto result = restore_config_backup(backups[selected], destination, root(), account);
                    message = tr_value("backup_restored", account);
                    if (!result.cleanup_error.empty()) message += "\n" + tr_value("backup_cleanup_failed", result.cleanup_error);
                    if (!result.retained_backup.empty()) message += "\n" + tr_value("backup_retained", result.retained_backup.u8string());
                    {
                        std::lock_guard<std::mutex> lock(g_data_mutex);
                        status_msg = "backup_restored";
                        status_detail = account;
                    }
                    refresh();
                } catch (const ConfigFileError& error) { message = tr_value(error.key.c_str(), error.detail); }
                catch (const std::exception& error) { message = tr_value("error", error.what()); }
                confirming = false;
            }
        } else {
            ImGui::TextWrapped("%s", tr("backup_retention"));
            ImGui::BeginChild("##backup_list", {0.f, 146.f}, ImGuiChildFlags_Borders);
            if (backups.empty()) ImGui::TextWrapped("%s", tr("backup_empty"));
            for (int i = 0; i < static_cast<int>(backups.size()); ++i) {
                ImGui::PushID(i);
                if (ImGui::Selectable(backups[i].date.c_str(), selected == i)) selected = i;
                ImGui::PopID();
            }
            ImGui::EndChild();
            ImGui::BeginDisabled(selected < 0);
            if (ImGui::Button(tr("backup_restore"), {-1.f, 34.f})) confirming = true;
            ImGui::EndDisabled();
        }
        if (!message.empty()) ImGui::TextWrapped("%s", message.c_str());
        if (ImGui::Button(tr("update_close"), {-1.f, 34.f})) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
}
