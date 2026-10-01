#pragma once
#include <string>

std::string exe_dir();
std::string config_dir();
std::string browse_for_folder(const char* title);
bool open_external(const std::string& value);

struct FontPaths {
    std::string regular;
    std::string bold;
};
FontPaths find_font_paths();

#ifdef _WIN32
std::string read_registry_settings();
bool write_registry_settings(const std::string& settings);
#endif
