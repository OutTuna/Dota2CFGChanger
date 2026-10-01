#include "avatar_data.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <cassert>
#include <iostream>
namespace fs = std::filesystem;
std::string folder;
std::string config_dir() { return folder; }
int main(int argc, char** argv) {
    auto root = fs::temp_directory_path() / ("dotamanager-avatars-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    folder = root.u8string();
    fs::create_directories(root / "avatar_cache");
    fs::copy_file(fs::u8path(argv[1]), root / "avatar_cache/42.img");
    avatar_data_request("42");
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    std::vector<AvatarPixels> ready;
    while (ready.empty()) {
        ready = avatar_data_take_ready();
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    assert(ready.size() == 1 && ready[0].id == "42");
    assert(ready[0].width > 0 && ready[0].height > 0);
    assert(ready[0].pixels.size() == static_cast<std::size_t>(ready[0].width) * ready[0].height * 4);
    assert(avatar_data_take_ready().empty());
    { std::ofstream f(root / "avatar_cache/43.img"); f << "invalid image"; }
    avatar_data_request("43");
    avatar_data_shutdown();
    assert(avatar_data_take_ready().empty());
    fs::remove_all(root);
    std::cout << "Avatar cache and background decoding work without a renderer\n";
}
