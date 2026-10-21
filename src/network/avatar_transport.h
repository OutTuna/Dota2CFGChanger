#pragma once
#include <string>
#include <functional>

std::string download_avatar(const std::string& url, const std::function<bool()>& active);

inline constexpr std::size_t MAX_AVATAR_BYTES = 4 * 1024 * 1024;
