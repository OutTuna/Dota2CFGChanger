#include "release_info.h"
#include <nlohmann/json.hpp>
#include <array>
#include <regex>
#include <stdexcept>
#include <limits>

namespace {
std::array<unsigned long, 4> version_parts(const std::string& value) {
    static const std::regex pattern(R"(^v?([0-9]+)(?:\.([0-9]+))?(?:\.([0-9]+))?(?:\.([0-9]+))?$)");
    std::smatch match;
    if (!std::regex_match(value, match, pattern)) throw std::runtime_error("Invalid version");
    std::array<unsigned long, 4> result{};
    for (int i = 0; i < 4; ++i) if (match[i + 1].matched) result[i] = std::stoul(match[i + 1]);
    return result;
}
}

bool newer_version(const std::string& candidate, const std::string& current) {
    return version_parts(candidate) > version_parts(current);
}

ReleaseInfo parse_release_info(const std::string& body, bool windows) {
    auto release = nlohmann::json::parse(body);
    if (release.value("draft", false)) throw std::runtime_error("Release is a draft");
    ReleaseInfo result;
    auto tag = release.value("tag_name", "");
    if (tag != "latest") throw std::runtime_error("Unexpected release tag");
    std::smatch match;
    auto name = release.value("name", "");
    static const std::regex pattern(R"(^Latest Build \((v[0-9]+(?:\.[0-9]+){0,3})\)$)");
    if (!std::regex_match(name, match, pattern)) throw std::runtime_error("Release version is missing");
    result.version = match[1];
    version_parts(result.version);
    for (const auto& asset : release.at("assets")) {
        auto asset_name = asset.value("name", "");
        if (asset_name != (windows ? "DotaManager.exe" : "Dota2_CFG_Changer-x86_64.AppImage")) continue;
        if (asset.value("state", "") != "uploaded") continue;
        auto url = asset.value("browser_download_url", "");
        std::string expected = std::string(REPOSITORY_URL) + "/releases/download/latest/" + asset_name;
        if (url != expected) throw std::runtime_error("Unexpected asset URL");
        if (!asset.contains("digest") || !asset["digest"].is_string()) continue;
        auto digest = asset["digest"].get<std::string>();
        if (!std::regex_match(digest, std::regex("sha256:[0-9a-f]{64}"))) continue;
        auto size = asset.at("size").get<std::int64_t>();
        if (size <= 0 || size > 512LL * 1024 * 1024) throw std::runtime_error("Invalid asset size");
        result.asset_name = asset_name;
        result.download_url = url;
        result.digest = digest.substr(7);
        result.size = static_cast<std::uint64_t>(size);
        break;
    }
    return result;
}
