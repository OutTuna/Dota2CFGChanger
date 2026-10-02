#!/usr/bin/env bash
set -euo pipefail
if [[ ! -x /usr/bin/DotaManager ]]; then
    echo "Installed executable is missing: /usr/bin/DotaManager" >&2
    exit 1
fi
check_dir=$(mktemp -d)
trap 'rm -rf "$check_dir"' EXIT
export XDG_CONFIG_HOME="$check_dir/config"
export LIBGL_ALWAYS_SOFTWARE=1
cd "$check_dir"
xvfb-run -a -s '-screen 0 1024x768x24' bash <<'RUN'
set -euo pipefail
/usr/bin/DotaManager > application.log 2>&1 &
app_pid=$!
cleanup() {
    kill "$app_pid" 2>/dev/null || true
    wait "$app_pid" 2>/dev/null || true
    cat application.log
}
trap cleanup EXIT
for attempt in $(seq 1 40); do
    if ! kill -0 "$app_pid" 2>/dev/null; then
        echo 'Installed package exited before opening its window' >&2
        exit 1
    fi
    if xwininfo -root -tree | grep -F 'Dota 2 CFG Changer' > /dev/null; then
        sleep 3
        kill -0 "$app_pid"
        echo 'Installed Arch package opened its main window'
        exit 0
    fi
    sleep 0.5
done
echo 'Installed Arch package did not open its window' >&2
exit 1
RUN
