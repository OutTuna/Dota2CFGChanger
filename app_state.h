#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>

inline const std::string DOTA_ID = "570";
inline constexpr size_t PATH_BUF_SIZE = 1024;
inline char src_path[PATH_BUF_SIZE] = "";
inline char dst_path[PATH_BUF_SIZE] =
#ifdef _WIN32
    "C:\\Program Files (x86)\\Steam\\userdata";
#else
    "";
#endif

inline std::mutex g_data_mutex;
inline std::map<std::string, std::string> nick_cache;
inline std::vector<std::string> src_list;
inline std::vector<std::string> dst_list;
inline std::string status_msg = "Ready";
inline std::atomic<int> selected_src{-1};
inline std::atomic<int> selected_dst{-1};
inline std::atomic<bool> g_scanning{false};
inline int g_theme = 0;

struct SuccessInfo {
    bool        show     = false;
    bool        closing  = false;
    float       anim_t   = 0.0f;
    float       close_t  = 0.0f;
    std::string src_id,  dst_id;
    std::string src_nick, dst_nick;
    std::string src_folder, dst_folder;
};

inline SuccessInfo g_success;
