#pragma once
#include "file_ops.h"
#include <vector>

enum class ConfigChange { Add, Replace, Unchanged, Remove };
struct ConfigChangeEntry {
    std::filesystem::path relative;
    ConfigChange change;
    bool directory;
    std::uintmax_t bytes;
};
struct ConfigPreview {
    std::filesystem::path source, destination;
    ConfigTree source_state, destination_state;
    bool destination_exists = false;
    std::vector<ConfigChangeEntry> entries;
    std::size_t affected_files = 0;
    std::uintmax_t copy_bytes = 0;
    bool changes = false;
};
ConfigPreview preview_config_transfer(const std::filesystem::path& source,
    const std::filesystem::path& destination);
void validate_config_preview(const ConfigPreview& preview, const std::filesystem::path& source,
    const std::filesystem::path& destination);
