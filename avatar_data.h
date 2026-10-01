#pragma once
#include <string>
#include <vector>

struct AvatarPixels {
    std::string id;
    std::vector<unsigned char> pixels;
    int width;
    int height;
};
void avatar_data_request(const std::string& id);
std::vector<AvatarPixels> avatar_data_take_ready();
void avatar_data_shutdown();
