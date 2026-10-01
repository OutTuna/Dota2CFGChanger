#pragma once
#include <filesystem>
#include <string>
std::string file_sha256(const std::filesystem::path& path);
