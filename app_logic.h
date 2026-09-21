#pragma once
#include <string>

void load_settings();
void save_settings();
void scan_thread();
void copy_config();

std::string browse_for_folder(const char* title);
