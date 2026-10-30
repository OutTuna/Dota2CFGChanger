#include "app.h"
#include "file_ops.h"
#include "localization.h"
#include <nlohmann/json.hpp>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
std::string folder;
std::string config_dir() { return folder; }
#ifdef _WIN32
std::string registry;
std::string read_registry_settings() { return registry; }
bool write_registry_settings(const std::string& value) { registry = value; return true; }
#endif
namespace steam_api {
std::string fetch_profile_xml(long long, int) { return {}; }
void cancel_requests() {}
}
std::string read(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}
void put_settings(const std::string& text) {
#ifdef _WIN32
    registry = text;
#else
    std::ofstream(fs::u8path(folder) / SETTINGS_FILE, std::ios::binary) << text;
#endif
}
std::string settings() {
#ifdef _WIN32
    return registry;
#else
    return read(fs::u8path(folder) / SETTINGS_FILE);
#endif
}
int main() {
    auto root = fs::temp_directory_path() / ("dota-settings-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root); folder = root.u8string();
    for (const auto* value : {"1e300", "1.5", "-1", "18446744073709551615", "true", "null", "5", "\"1\""}) {
        auto text = std::string("{\"theme\":") + value + "}";
        put_settings(text); load_settings();
        assert(status_msg == "settings_load_failed");
        save_settings(); assert(settings() == text);
    }
    put_settings("{\"theme\":4,\"language\":\"uk\"}"); load_settings();
    assert(g_theme == 4 && std::string(language_code()) == "uk");
#ifdef _WIN32
    registry.clear();
#endif
    std::ofstream(root / SETTINGS_FILE, std::ios::binary).close();
    load_settings(); save_settings();
    assert(status_msg == "settings_load_failed" && read(root / SETTINGS_FILE).empty());
    assert(set_config_path(src_path, (root / "empty-recovery").u8string()));
    save_settings();
    auto recovery = root / (std::string(SETTINGS_FILE) + ".invalid");
    assert(fs::is_regular_file(recovery) && read(recovery).empty());
    fs::remove(recovery);
    const std::string invalid = "{invalid";
    put_settings(invalid); load_settings(); save_settings();
    assert(settings() == invalid);
    assert(set_config_path(src_path, (root / "changed").u8string()));
    save_settings();
    assert(nlohmann::json::parse(settings()).at("src") == (root / "changed").u8string());
    assert(read(root / (std::string(SETTINGS_FILE) + ".invalid")) == invalid);
    put_settings("{another invalid"); load_settings();
    assert(set_config_path(src_path, (root / "another-change").u8string()));
    save_settings();
    assert(status_msg == "settings_save_failed" && settings() == "{another invalid");
    assert(read(root / (std::string(SETTINGS_FILE) + ".invalid")) == invalid);
    load_settings(); app_shutdown();
    assert(settings() == "{another invalid");
    fs::remove_all(root);
    std::cout << "Invalid numeric settings, unchanged shutdown and explicit recovery passed\n";
}
