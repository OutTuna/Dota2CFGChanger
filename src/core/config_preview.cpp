#include "config_preview.h"
#include <limits>

namespace fs = std::filesystem;
ConfigPreview preview_config_transfer(const fs::path& source, const fs::path& destination) {
    ConfigPreview result;
    result.source = fs::weakly_canonical(source);
    result.destination = fs::weakly_canonical(destination);
    if (config_path_contains(result.source, result.destination) || config_path_contains(result.destination, result.source))
        throw ConfigFileError("overlap");
    result.source_state = inspect_config_tree(source);
    reject_config_link(destination); reject_config_link(destination.parent_path());
    result.destination_exists = fs::exists(fs::symlink_status(destination));
    if (result.destination_exists) result.destination_state = inspect_config_tree(destination);
    result.changes = !result.destination_exists;
    for (const auto& [name, item] : result.source_state) {
        auto found = result.destination_state.find(name);
        auto change = ConfigChange::Add;
        if (found != result.destination_state.end()) {
            const auto& previous = found->second;
            change = item.directory == previous.directory && (item.directory ||
                (item.size == previous.size && config_files_equal(source / name, destination / name)))
                ? ConfigChange::Unchanged : ConfigChange::Replace;
        }
        if (item.size > std::numeric_limits<std::uintmax_t>::max() - result.copy_bytes)
            throw ConfigFileError("space_insufficient");
        result.copy_bytes += item.size;
        result.entries.push_back({name, change, item.directory, item.size});
        if (change != ConfigChange::Unchanged) {
            result.changes = true;
            if (!item.directory || (found != result.destination_state.end() && !found->second.directory)) ++result.affected_files;
        }
    }
    for (const auto& [name, item] : result.destination_state) {
        if (result.source_state.count(name)) continue;
        result.entries.push_back({name, ConfigChange::Remove, item.directory, item.size});
        result.changes = true;
        if (!item.directory) ++result.affected_files;
    }

    validate_config_preview(result, source, destination);
    return result;
}

void validate_config_preview(const ConfigPreview& preview, const fs::path& source, const fs::path& destination) {
    if (fs::weakly_canonical(source) != preview.source || fs::weakly_canonical(destination) != preview.destination ||
        inspect_config_tree(source) != preview.source_state ||
        fs::exists(fs::symlink_status(destination)) != preview.destination_exists ||
        (preview.destination_exists && inspect_config_tree(destination) != preview.destination_state))
        throw ConfigFileError("config_changed");
}
