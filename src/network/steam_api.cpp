#include "steam_api.h"
#include <cstring>

namespace steam_api {

static const long long STEAM64_BASE = 76561197960265728LL;

long long steam3_to_64(const std::string& steam3_id) {
    try {
        return std::stoll(steam3_id) + STEAM64_BASE;
    } catch (...) {
        return 0;
    }
}

static std::string decode_xml_entities(std::string s) {
    struct Entity { const char* from; char to; };
    static const Entity entities[] = {
        {"&lt;", '<'}, {"&gt;", '>'}, {"&quot;", '"'}, {"&apos;", '\''}, {"&amp;", '&'},
    };
    for (const auto& e : entities) {
        size_t pos = 0;
        const size_t from_len = std::strlen(e.from);
        while ((pos = s.find(e.from, pos)) != std::string::npos) {
            s.replace(pos, from_len, 1, e.to);
            pos += 1;
        }
    }
    return s;
}

std::string extract_tag(const std::string& xml, const std::string& tag) {
    const std::string open  = "<" + tag + ">";
    const std::string close = "</" + tag + ">";
    size_t s = xml.find(open);
    if (s == std::string::npos) return {};
    size_t e = xml.find(close, s + open.size());
    if (e == std::string::npos) return {};
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

    return decode_xml_entities(raw);
}

}

