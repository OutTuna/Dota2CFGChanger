#!/usr/bin/env bash
set -euo pipefail

appimage=$(realpath "${1:?AppImage path required}")
script_dir=$(cd "$(dirname "$0")" && pwd)
check_dir=$(mktemp -d)
trap 'rm -rf "$check_dir"' EXIT
chmod +x "$appimage"
cd "$check_dir"
"$appimage" --appimage-extract > extract.log
python3 "$script_dir/check-appimage-abi.py" "$check_dir/squashfs-root"
mkdir -p "$check_dir/home" "$check_dir/config" "$check_dir/cache"
export HOME="$check_dir/home"
export XDG_CONFIG_HOME="$check_dir/config"
export XDG_CACHE_HOME="$check_dir/cache"
export LIBGL_ALWAYS_SOFTWARE=1
export APPIMAGE_EXTRACT_AND_RUN=1
export APPIMAGE_CHECK_FILE="$appimage"

xvfb-run -a -s '-screen 0 1024x768x24' bash <<'RUN'
set -euo pipefail
"$APPIMAGE_CHECK_FILE" > application.log 2>&1 &
app_pid=$!
cleanup() {
    kill "$app_pid" 2>/dev/null || true
    wait "$app_pid" 2>/dev/null || true
    cat application.log
}
trap cleanup EXIT
for attempt in $(seq 1 40); do
    if ! kill -0 "$app_pid" 2>/dev/null; then
        echo 'AppImage exited before opening its window' >&2
        exit 1
    fi
    if xwininfo -root -tree | grep -F 'Dota 2 CFG Changer' > /dev/null; then
        sleep 3
        if ! kill -0 "$app_pid" 2>/dev/null; then
            echo 'AppImage exited after opening its window' >&2
            exit 1
        fi
        echo 'AppImage window opened and remained running'
        exit 0
    fi
    sleep 0.5
done
echo 'AppImage did not open its window within 20 seconds' >&2
exit 1
RUN

xvfb-run -a -s '-screen 0 1024x768x24' python3 "$script_dir/../tests/appimage_update_checks.py" "$appimage"
