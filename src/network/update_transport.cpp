#include "update_transport.h"
#include <cpr/cpr.h>
#include <fstream>
#include <stdexcept>

std::string fetch_update_release(const std::function<bool()>& active) {

    cpr::Session session;
    session.SetUrl(cpr::Url{"https://api.github.com/repos/OutTuna/Dota2CFGChanger/releases/tags/latest"});
    session.SetHeader(cpr::Header{{"User-Agent", "DotaManager"}, {"Accept", "application/vnd.github+json"}});
    session.SetTimeout(cpr::Timeout{10000}); session.SetConnectTimeout(cpr::ConnectTimeout{4000});
    session.SetVerifySsl(cpr::VerifySsl{true});
    session.SetRedirect(cpr::Redirect{false});
    session.SetProgressCallback(cpr::ProgressCallback{[&](cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, intptr_t) { return active(); }});
    std::string body;
    session.SetWriteCallback(cpr::WriteCallback{[&](const std::string_view& bytes, intptr_t) {
        constexpr std::size_t limit = 2 * 1024 * 1024;
        if (bytes.size() > limit - body.size()) return false;
        body.append(bytes.data(), bytes.size());
        return active();
    }});
    auto response = session.Get();
    if (response.status_code != 200 || response.error.code != cpr::ErrorCode::OK)
        throw std::runtime_error(response.status_code == 403 || response.status_code == 429 ? "GitHub API rate limit" : "GitHub request failed");
    return body;
}

void download_update_asset(const ReleaseInfo& release, const std::filesystem::path& partial,
    const std::function<bool(std::uint64_t)>& progress) {
    const auto expected = std::string(REPOSITORY_URL) + "/releases/download/latest/" + release.asset_name;
    if ((release.asset_name != "DotaManager.exe" && release.asset_name != "Dota2_CFG_Changer-x86_64.AppImage") ||
        release.download_url != expected) throw std::runtime_error("Unexpected update URL");
    std::ofstream output(partial, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("Cannot create download file");
    std::uint64_t written = 0;
    cpr::Session session;
    session.SetUrl(cpr::Url{release.download_url});
    session.SetVerifySsl(cpr::VerifySsl{true});
    session.SetRedirect(cpr::Redirect{5L});
    auto handle = session.GetCurlHolder()->handle;
#if LIBCURL_VERSION_NUM >= 0x075500
    if (curl_easy_setopt(handle, CURLOPT_PROTOCOLS_STR, "https") != CURLE_OK ||
        curl_easy_setopt(handle, CURLOPT_REDIR_PROTOCOLS_STR, "https") != CURLE_OK)
#else
    if (curl_easy_setopt(handle, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS) != CURLE_OK ||
        curl_easy_setopt(handle, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS) != CURLE_OK)
#endif
        throw std::runtime_error("Cannot enforce HTTPS for update download");
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
