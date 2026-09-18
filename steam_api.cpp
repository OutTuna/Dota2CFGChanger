#include "steam_api.h"
#include <cpr/cpr.h>

namespace steam_api {

static const long long STEAM64_BASE = 76561197960265728LL;

long long steam3_to_64(const std::string& steam3_id) {
    try {
        return std::stoll(steam3_id) + STEAM64_BASE;
    } catch (...) {
        return 0;
    }
}

std::string fetch_profile_xml(long long steam64, int timeout_ms) {
    if (steam64 <= 0) return {};
    try {
        auto r = cpr::Get(
            cpr::Url{ "https://steamcommunity.com/profiles/" + std::to_string(steam64) + "?xml=1" },
            cpr::Timeout{ timeout_ms });
        if (r.status_code == 200) return r.text;
    } catch (...) {}
    return {};
}

std::string extract_tag(const std::string& xml, const std::string& tag) {
    const std::string open  = "<" + tag + ">";
    const std::string close = "</" + tag + ">";
    size_t s = xml.find(open);
    size_t e = xml.find(close);
    if (s == std::string::npos || e == std::string::npos || e < s) return {};
    std::string raw = xml.substr(s + open.size(), e - s - open.size());
    const std::string cdata_open  = "<![CDATA[";
    const std::string cdata_close = "]]>";
    size_t p = raw.find(cdata_open);
    if (p != std::string::npos) raw.replace(p, cdata_open.size(), "");
    p = raw.find(cdata_close);
    if (p != std::string::npos) raw.replace(p, cdata_close.size(), "");
    while (!raw.empty() && (raw.front() == ' ' || raw.front() == '\n' || raw.front() == '\r'))
        raw.erase(raw.begin());
    while (!raw.empty() && (raw.back() == ' ' || raw.back() == '\n' || raw.back() == '\r'))
        raw.pop_back();

    return raw;
}

}
