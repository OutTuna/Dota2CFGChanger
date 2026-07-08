#pragma once
#include <string>
#include <vector>
#include <map>

//inline const std::string SETTINGS_FILE = "settings.json";
//inline const std::string CACHE_FILE    = "nicknames.json";
inline const std::string DOTA_ID       = "570";

inline char src_path[256] = "";
inline char dst_path[256] = "C:\\Program Files (x86)\\Steam\\userdata";

inline std::map<std::string, std::string> nick_cache;
inline std::vector<std::string> src_list;
inline std::vector<std::string> dst_list;

inline std::string status_msg = "Ready";
inline int selected_src = -1;
inline int selected_dst = -1;

inline int  g_theme = 0;

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