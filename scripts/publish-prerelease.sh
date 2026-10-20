#!/usr/bin/env bash

set -euo pipefail
fail() { echo "$1" >&2; exit 1; }
case "${RELEASE_CHANNEL:?Missing channel}" in
  alpha) channel_title=Alpha ;;
  *) echo 'Only the alpha channel is allowed' >&2; exit 1 ;;
esac
[[ "${GITHUB_REF:?Missing ref}" == "refs/heads/$RELEASE_CHANNEL" ]] || fail "Invalid prerelease input or missing artifact"
[[ "${VERSION:?Missing version}" =~ ^v[0-9]+\.[0-9]+$ ]] || fail "Invalid prerelease input or missing artifact"
[[ "${GITHUB_RUN_ID:?Missing run ID}" =~ ^[0-9]+$ ]] || fail "Invalid prerelease input or missing artifact"
[[ "${GITHUB_RUN_ATTEMPT:?Missing run attempt}" =~ ^[0-9]+$ ]] || fail "Invalid prerelease input or missing artifact"
[[ "${GITHUB_SHA:?Missing commit}" =~ ^[0-9a-f]{40}$ ]] || fail "Invalid prerelease input or missing artifact"
[[ -f DotaManager-Windows/DotaManager.exe ]] || fail "Invalid prerelease input or missing artifact"
shopt -s nullglob
images=(DotaManager-Linux/*.AppImage)
[[ ${#images[@]} -eq 1 ]] || fail "Invalid prerelease input or missing artifact"
tag="${RELEASE_CHANNEL}-${VERSION}-${GITHUB_RUN_ID}-${GITHUB_RUN_ATTEMPT}"
gh release create "$tag" \
  --target "$GITHUB_SHA" \
  --title "${channel_title} Build ($VERSION, CI $GITHUB_RUN_ID.$GITHUB_RUN_ATTEMPT)" \
  --prerelease --latest=false \
  --notes "Testing build from $RELEASE_CHANNEL at $GITHUB_SHA. Not offered by stable automatic updates." \
  DotaManager-Windows/DotaManager.exe "${images[0]}"
