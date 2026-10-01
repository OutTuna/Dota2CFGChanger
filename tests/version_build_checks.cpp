#include "updates.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2 || std::string(app_version()) != argv[1]) {
        std::cerr << "Compiled application version differs from the release version: "
                  << app_version() << '\n';
        return 1;
    }
    if (newer_version(argv[1], app_version())) return 1;
    std::cout << "Application and release versions match: " << app_version() << '\n';
}
