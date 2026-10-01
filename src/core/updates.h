#pragma once
#include "release_info.h"
#include <string>

enum class UpdatePhase { Idle, Checking, Available, Current, Downloading, Downloaded, Failed };
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
void updates_check(bool manual);
void updates_download();
void updates_dismiss();
UpdateSnapshot updates_snapshot();
void updates_shutdown();
