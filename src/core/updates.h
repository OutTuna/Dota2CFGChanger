#pragma once
#include "release_info.h"
#include "app_version.h"
#include <string>

enum class UpdatePhase { Idle, Checking, Available, Current, Downloading, Downloaded, Installing, Failed };
struct UpdateSnapshot {
    UpdatePhase phase = UpdatePhase::Idle;
    ReleaseInfo release;
    float progress = 0;
    bool popup = false;
    std::string error_key;
    std::string detail;
    std::string downloaded_path;
};
const char* app_version();
inline const char* app_release_channel() { return DOTA_RELEASE_CHANNEL; }
inline std::string app_build_version_label() {
    const std::string channel = app_release_channel();
    return std::string(app_version()) + (channel == "stable" ? "" : " " + channel);
}
void updates_check(bool manual);
void updates_download();
void updates_dismiss();
UpdateSnapshot updates_snapshot();
UpdatePhase updates_phase();
void updates_shutdown();
void updates_request_stop();

void updates_install();
bool updates_should_exit();
void updates_install_error(const std::string& detail);

bool updates_self_install_enabled();
