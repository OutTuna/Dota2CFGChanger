#include "steam_api.h"
#include <cpr/cpr.h>
#include <atomic>

namespace {
std::atomic<bool> stopping{false};
}

namespace steam_api {
void cancel_requests() { stopping = true; }

std::string fetch_profile_xml(long long steam64, int timeout_ms) {
    if (steam64 <= 0 || stopping.load()) return {};
    try {
        auto response = cpr::Get(
            cpr::Url{"https://steamcommunity.com/profiles/" + std::to_string(steam64) + "?xml=1"},
            cpr::Header{{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36"}, {"Accept", "text/xml,application/xml,*/*"}},
            cpr::Timeout{timeout_ms}, cpr::ConnectTimeout{4000},
            cpr::Redirect{cpr::PostRedirectFlags::POST_ALL},
            cpr::ProgressCallback{[](cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, intptr_t) { return !stopping.load(); }});
        if (!stopping.load() && response.status_code == 200 && response.error.code == cpr::ErrorCode::OK)
            return std::move(response.text);
    } catch (...) {}
    return {};
}
}
