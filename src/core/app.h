#pragma once

#include <string>
#include "platform.h"
#include "steam_api.h"
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include "ui_events.h"
#include "file_tasks.h"

constexpr int PATH_BUF_SIZE = 4096;

inline constexpr const char* SETTINGS_FILE     = "settings.json";
inline constexpr const char* CACHE_FILE        = "nick_cache.json";
inline constexpr const char* AVATAR_URL_CACHE_FILE = "avatar_url_cache.json";
inline constexpr const char* AVATAR_DISK_DIR   = "avatar_cache";
inline constexpr const char* DOTA_ID           = "570";

inline char src_path[PATH_BUF_SIZE] = {};
inline char dst_path[PATH_BUF_SIZE] = {};
bool set_config_path(char* target, const std::string& path);

inline std::atomic<int> g_theme{0};

inline std::mutex g_data_mutex;

inline std::vector<std::string> src_list;
inline std::vector<std::string> dst_list;
inline std::map<std::string, std::string> nick_cache;
inline std::map<std::string, std::string> avatar_url_cache;
inline std::string status_msg = "ready";
inline std::string status_detail;

inline std::atomic<int> selected_src{ -1 };
inline std::atomic<int> selected_dst{ -1 };
inline std::atomic<bool> g_scanning{ false };

inline std::atomic<unsigned long long> g_ui_generation{1};
inline std::atomic<unsigned long long> g_account_generation{1};
inline std::atomic<unsigned long long> g_cache_generation{1};
inline void app_status_changed() {
    ++g_ui_generation;
    ui_events::request_redraw();
}
inline void app_ui_changed() {
    ++g_account_generation;
    app_status_changed();
}

struct AccountView {
    std::string id;
    std::string label;
};
struct UiSnapshot {
    unsigned long long generation = 0;
    unsigned long long account_generation = 0;
    std::vector<AccountView> source;
    std::vector<AccountView> destination;
    std::string status;
    std::string detail;
};
const UiSnapshot& app_ui_snapshot();

struct SuccessPopupState {
    bool show = false;
    bool closing = false;
    float anim_t = 0.f;
    float close_t = 0.f;
    std::string src_id;
    std::string dst_id;
    std::string src_nick;
    std::string dst_nick;
    std::string src_label;
    std::string dst_label;
    std::string src_folder;
    std::string dst_folder;
};

inline SuccessPopupState g_success;

void load_settings();
void save_settings();
bool save_profile_caches();
bool ensure_profile_info(const std::string& id);
void copy_config();
std::string browse_for_folder(const char* title);

void start_scan();
void app_shutdown();
void app_request_stop();
