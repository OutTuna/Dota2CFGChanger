#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>

constexpr int PATH_BUF_SIZE = 256;

inline constexpr const char* SETTINGS_FILE = "settings.json";
inline constexpr const char* CACHE_FILE    = "nick_cache.json";
inline constexpr const char* DOTA_ID       = "570";

inline char src_path[PATH_BUF_SIZE] = "C:\\Program Files (x86)\\Steam\\userdata";
inline char dst_path[PATH_BUF_SIZE] = "C:\\Program Files (x86)\\Steam\\userdata";

inline int g_theme = 0;

inline std::mutex g_data_mutex;

inline std::vector<std::string> src_list;
inline std::vector<std::string> dst_list;
inline std::map<std::string, std::string> nick_cache;
inline std::string status_msg = "Ready";

inline std::atomic<int> selected_src{ -1 };
inline std::atomic<int> selected_dst{ -1 };
inline std::atomic<bool> g_scanning{ false };

struct SuccessPopupState {
    bool show = false;
    bool closing = false;
    float anim_t = 0.f;
    float close_t = 0.f;
    std::string src_id;
    std::string dst_id;
    std::string src_nick;
    std::string dst_nick;
    std::string src_folder;
    std::string dst_folder;
};

inline SuccessPopupState g_success;
