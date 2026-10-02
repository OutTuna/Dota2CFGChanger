#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

struct UpdateProcess {
    std::uint64_t id = 0;
    std::uintptr_t handle = 0;
};
std::filesystem::path running_update_target();
std::uint64_t current_process_id();
UpdateProcess watch_update_parent(std::uint64_t id);
UpdateProcess launch_update_process(const std::filesystem::path& executable,
    const std::vector<std::string>& arguments);
bool update_process_exited(const UpdateProcess& process, unsigned milliseconds);
void stop_update_process(const UpdateProcess& process);
void release_update_process(UpdateProcess& process);
std::vector<std::string> update_arguments(int argc, char** argv);
void show_update_install_error(const std::string& detail);
