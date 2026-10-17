#include "ui.h"
#include "app.h"
#include "backups.h"
#include "file_ops.h"
#include "localization.h"
#include "updates.h"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
namespace {
bool requested = false;
bool confirming = false;
std::string account;
fs::path destination;
std::string destination_label;
std::vector<ConfigBackup> backups;
int selected = -1;
std::string message;
fs::path root() { return fs::u8path(config_dir()) / "backups"; }
std::vector<ConfigBackup> find_backups(const fs::path& folder, const std::string& id, const fs::path& destination) {
    auto result = list_config_backups(folder, id);
    auto target = fs::weakly_canonical(destination);
    result.erase(std::remove_if(result.begin(), result.end(), [&](const ConfigBackup& backup) {
        return fs::weakly_canonical(backup.destination) != target;
    }), result.end());
    return result;
}
FileTaskCompletion failed(const std::string& key, const std::string& detail) {
    return [key, detail] { message = tr_value(key.c_str(), detail); confirming = false; };
}
}

void ui_open_backups() {
    if (g_scanning.load() || file_tasks_busy() || updates_phase() == UpdatePhase::Installing) return;
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        int index = selected_dst.load();
        if (index < 0 || index >= static_cast<int>(dst_list.size())) return;
        account = dst_list[index];
        destination = fs::u8path(dst_path) / account / DOTA_ID;
    }
    destination_label = destination.u8string();
    confirming = false;
    message.clear();
    backups.clear();
    selected = -1;
    file_tasks_start([folder = root(), id = account, target = destination]() -> FileTaskCompletion {
        try {
            auto result = find_backups(folder, id, target);
            return [result = std::move(result)]() mutable { backups = std::move(result); };
        } catch (const std::exception& error) { return failed("error", error.what()); }
    });
    requested = true;
}

void ui_render_backups_popup() {
    const char* id = "###backups_popup";
    if (!requested && !ImGui::IsPopupOpen(id)) return;
    if (requested) { ImGui::OpenPopup(id); requested = false; }
    auto display = ImGui::GetIO().DisplaySize;
    float width = std::min(500.f, display.x - 32.f);
    ImGui::SetNextWindowPos({display.x * .5f, display.y * .5f}, ImGuiCond_Always, {.5f, .5f});
    ImGui::SetNextWindowSizeConstraints({width, 0.f}, {width, display.y - 32.f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {20.f, 18.f});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8.f, 10.f});
    bool open = true;
    static int title_language = -1;
    static std::string title;
    if (title_language != language_index()) {
        title_language = language_index();
        title = std::string(tr("backups")) + id;
    }
    if (ImGui::BeginPopupModal(title.c_str(), &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        ImGui::TextWrapped("%s", tr_value("backup_account", account).c_str());
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("%s", destination_label.c_str());
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", destination_label.c_str());
        ImGui::Separator();
        const bool busy = file_tasks_busy() || updates_phase() == UpdatePhase::Installing;
        if (busy) { ImGui::TextUnformatted(tr("working")); ImGui::BeginDisabled(); }
        if (confirming && selected >= 0 && selected < static_cast<int>(backups.size())) {
            ImGui::TextWrapped("%s", tr_value("backup_restore_warning", backups[selected].date).c_str());
            if (ImGui::Button(tr("cancel"), {-1.f, 34.f})) confirming = false;
            if (ImGui::Button(tr("backup_restore"), {-1.f, 34.f})) {
                file_tasks_start([backup = backups[selected], target = destination, folder = root(), id = account]() -> FileTaskCompletion {
                    try {
                        auto result = restore_config_backup(backup, target, folder, id);
                        auto refreshed = find_backups(folder, id, target);
                        return [result, refreshed = std::move(refreshed), id]() mutable {
                            message = tr_value("backup_restored", id);
                            if (!result.cleanup_error.empty()) message += "\n" + tr_value("backup_cleanup_failed", result.cleanup_error);
                            if (!result.retained_backup.empty()) message += "\n" + tr_value("backup_retained", result.retained_backup.u8string());
                            {
                                std::lock_guard<std::mutex> lock(g_data_mutex);
                                status_msg = "backup_restored";
                                status_detail = id;
                                app_status_changed();
                            }
                            backups = std::move(refreshed);
                            selected = -1;
                            confirming = false;
                        };
                    } catch (const ConfigFileError& error) { return failed(error.key, error.detail); }
                    catch (const std::exception& error) { return failed("error", error.what()); }
                });
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
        if (busy) ImGui::EndDisabled();
        if (!message.empty()) ImGui::TextWrapped("%s", message.c_str());
        if (ImGui::Button(tr("update_close"), {-1.f, 34.f})) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
}
