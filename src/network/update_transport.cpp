#include "update_transport.h"
#include <cpr/cpr.h>
#include <fstream>
#include <stdexcept>

std::string fetch_update_release(const std::function<bool()>& active) {
    auto response = cpr::Get(
        cpr::Url{"https://api.github.com/repos/OutTuna/Dota2CFGChanger/releases/tags/latest"},
        cpr::Header{{"User-Agent", "DotaManager"}, {"Accept", "application/vnd.github+json"}},
        cpr::Timeout{10000}, cpr::ConnectTimeout{4000},
        cpr::ProgressCallback{[&](cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, intptr_t) { return active(); }});
    if (response.status_code != 200 || response.error.code != cpr::ErrorCode::OK)
        throw std::runtime_error(response.status_code == 403 || response.status_code == 429 ? "GitHub API rate limit" : "GitHub request failed");
    return response.text;
}

void download_update_asset(const ReleaseInfo& release, const std::filesystem::path& partial,
    const std::function<bool(std::uint64_t)>& progress) {
    std::ofstream output(partial, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("Cannot create download file");
    std::uint64_t written = 0;
    cpr::Session session;
    session.SetUrl(cpr::Url{release.download_url});
    session.SetHeader(cpr::Header{{"User-Agent", "DotaManager"}});
    session.SetTimeout(cpr::Timeout{300000});
    session.SetConnectTimeout(cpr::ConnectTimeout{5000});
    session.SetProgressCallback(cpr::ProgressCallback{[&](cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t now, cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, intptr_t) {
        return progress(static_cast<std::uint64_t>(now));
    }});
    auto response = session.Download(cpr::WriteCallback{[&](const std::string_view& bytes, intptr_t) {
        if (bytes.size() > release.size - written) return false;
        output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        written += bytes.size();
        return static_cast<bool>(output);
    }});
    output.close();
    if (response.status_code != 200 || response.error.code != cpr::ErrorCode::OK || written != release.size || !output)
        throw std::runtime_error("Download incomplete");
}
