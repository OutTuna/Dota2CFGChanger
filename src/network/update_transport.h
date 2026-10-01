#pragma once
#include "release_info.h"
#include <filesystem>
#include <functional>
#include <string>

std::string fetch_update_release(const std::function<bool()>& active);
void download_update_asset(const ReleaseInfo& release, const std::filesystem::path& partial,
    const std::function<bool(std::uint64_t)>& progress);
