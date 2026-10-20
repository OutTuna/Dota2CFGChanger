#include "backups.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
namespace fs = std::filesystem;
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const fs::path root = fs::u8path(argv[1]);
    for (const char* scenario : {"identical", "changed"}) {
        auto folder = root / scenario;
        auto source = folder / "source/570", destination = folder / "destination/570", backups = folder / "backups";
        fs::create_directories(source); fs::create_directories(destination);
        const std::string data(4096, 'x');
        for (int i = 0; i < 128; ++i) {
            std::ofstream(source / std::to_string(i)) << data;
            std::ofstream(destination / std::to_string(i)) << data;
        }
        std::vector<double> samples;
        for (int i = 0; i < 7; ++i) {
            if (std::string(scenario) == "changed") std::ofstream(source / "0") << std::string(4096, static_cast<char>('a' + i));
            const auto start = std::chrono::steady_clock::now();
            copy_config_with_backup(source, destination, backups, "42");
            samples.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
        }
        std::sort(samples.begin(), samples.end());
        std::cout << scenario << "_median_ms=" << samples[samples.size() / 2] << "\n";
    }
}
