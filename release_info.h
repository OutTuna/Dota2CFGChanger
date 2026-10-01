#pragma once
#include <string>
#include <cstdint>

inline constexpr const char* REPOSITORY_URL = "https://github.com/OutTuna/Dota2CFGChanger";
inline constexpr const char* RELEASE_URL = "https://github.com/OutTuna/Dota2CFGChanger/releases/tag/latest";
struct ReleaseInfo {
    std::string version;
    std::string asset_name;
    std::string download_url;
    std::string digest;
    std::uint64_t size = 0;
};
bool newer_version(const std::string& candidate, const std::string& current);
ReleaseInfo parse_release_info(const std::string& body, bool windows);
