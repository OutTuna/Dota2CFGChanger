#include "release_info.h"
#include "checksum.h"
#include <nlohmann/json.hpp>
#include <cassert>
#include <fstream>
#include <iostream>
#include <chrono>

int main() {
    using json = nlohmann::json;
    assert(newer_version("v1.10", "1.9"));
    assert(!newer_version("v1.9", "1.10"));
    assert(!newer_version("1.9.0", "v1.9"));
    assert(newer_version("2.0", "1.999"));
    bool invalid = false;
    try { newer_version("latest", "1.9"); } catch (...) { invalid = true; }
    assert(invalid);
    json release = {{"draft", false}, {"tag_name", "latest"}, {"name", "Latest Build (v1.10)"}, {"assets", json::array()}};
    auto parsed = parse_release_info(release.dump(), true);
    assert(parsed.version == "v1.10" && parsed.download_url.empty());
    json asset = {{"name", "DotaManager.exe"}, {"state", "uploaded"}, {"size", 3}, {"digest", "sha256:ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
        {"browser_download_url", "https://github.com/OutTuna/Dota2CFGChanger/releases/download/latest/DotaManager.exe"}};
    release["assets"].push_back(asset);
    parsed = parse_release_info(release.dump(), true);
    assert(parsed.size == 3 && parsed.asset_name == "DotaManager.exe");
    assert(parse_release_info(release.dump(), false).download_url.empty());
    auto reject = [&](const json& value) { bool failed = false; try { parse_release_info(value.dump(), true); } catch (...) { failed = true; } assert(failed); };
    auto bad = release; bad["assets"][0]["browser_download_url"] = "https://example.com/update.exe"; reject(bad);
    bad = release; bad["assets"][0]["digest"] = nullptr;
    assert(parse_release_info(bad.dump(), true).download_url.empty());
    bad = release; bad["assets"][0]["size"] = -1; reject(bad);
    bad = release; bad["draft"] = true; reject(bad);
    bad = release; bad["name"] = "Latest Build (v1.10/../bad)"; reject(bad);
    auto path = std::filesystem::temp_directory_path() / ("dotamanager-hash-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    { std::ofstream f(path, std::ios::binary); f << "abc"; }
    assert(file_sha256(path) == parsed.digest);
    { std::ofstream f(path, std::ios::binary); }
    assert(file_sha256(path) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    std::filesystem::remove(path);
    std::cout << "Versions, release validation and SHA-256 passed\n";
}
