#pragma once
#include "release_info.h"
#include <filesystem>
#include <functional>
#include <vector>

void replace_update_executable(const std::filesystem::path& staged,
    const std::filesystem::path& target, const ReleaseInfo& release,
    const std::function<bool()>& launch);
void prepare_update_install(const std::filesystem::path& downloaded,
    const ReleaseInfo& release, const std::function<bool()>& active);
int update_helper_dispatch(const std::vector<std::string>& arguments);
void update_install_confirm_started(const std::vector<std::string>& arguments);
std::string update_install_startup_error(const std::vector<std::string>& arguments);
