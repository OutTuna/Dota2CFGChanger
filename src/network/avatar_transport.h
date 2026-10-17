#pragma once
#include <string>
#include <functional>

std::string download_avatar(const std::string& url, const std::function<bool()>& active);
