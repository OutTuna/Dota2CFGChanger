#pragma once
#include <string>
namespace steam_api {
long long steam3_to_64(const std::string& steam3_id);
std::string fetch_profile_xml(long long steam64, int timeout_ms);
std::string extract_tag(const std::string& xml, const std::string& tag);
}
