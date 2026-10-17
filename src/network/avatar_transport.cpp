#include "avatar_transport.h"
#include <cpr/cpr.h>

std::string download_avatar(const std::string& url, const std::function<bool()>& active) {
    if (!active()) return {};
    auto response = cpr::Get(cpr::Url{url}, cpr::Header{{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36"}},
        cpr::Timeout{8000}, cpr::ConnectTimeout{4000},
        cpr::Redirect{cpr::PostRedirectFlags::POST_ALL},
        cpr::ProgressCallback{[&](cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, intptr_t) { return active(); }});
    if (!active() || response.status_code != 200 || response.error.code != cpr::ErrorCode::OK) return {};
    return std::move(response.text);
}
