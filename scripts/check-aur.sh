#!/usr/bin/env bash
set -euo pipefail
case "${1:?Use prepare or build}" in
prepare)
    pacman -Syu --noconfirm --needed base-devel cmake ninja nlohmann-json curl glfw \
        libglvnd openssl ttf-dejavu hicolor-icon-theme python desktop-file-utils namcap \
        xorg-server-xvfb xorg-xauth xorg-xwininfo mesa
    useradd --create-home builder
    mkdir -p /work/project /work/package /work/artifacts
    tar -C /workspace --exclude=.git -cf - . | tar -C /work/project -xf -
    cp -a /workspace/packaging/aur/. /work/package/
    chown -R builder:builder /work
    cd /work/package
    runuser -u builder -- makepkg --printsrcinfo > /work/artifacts/generated.SRCINFO
    diff -u .SRCINFO <(sed '/^#/d' /work/artifacts/generated.SRCINFO)
    runuser -u builder -- makepkg --nobuild --noconfirm
    version=$(bash -c 'source ./PKGBUILD; printf "%s" "$pkgver"')
    python /work/project/scripts/check-aur-sources.py "/work/package/src/Dota2CFGChanger-$version" /work/project
    ;;
build)
    cd /work/package
    runuser -u builder -- env CMAKE_BUILD_PARALLEL_LEVEL=4 MAKEFLAGS=-j4 makepkg --cleanbuild --noconfirm
    packages=(dota2cfgchanger-[0-9]*-x86_64.pkg.tar.zst)
    [[ ${#packages[@]} -eq 1 && -f "${packages[0]}" ]]
    namcap "${packages[0]}" 2>&1 | tee /work/artifacts/namcap.log
    if grep -E '(^|[[:space:]])E:' /work/artifacts/namcap.log; then exit 1; fi
    pacman -U --noconfirm "${packages[0]}"
    for file in /usr/bin/DotaManager /usr/share/applications/dota2cfgchanger.desktop \
        /usr/share/icons/hicolor/32x32/apps/dota2cfgchanger.png \
        /usr/share/licenses/dota2cfgchanger/LICENSE; do
        [[ -f "$file" ]]
        pacman -Qo "$file"
    done
    desktop-file-validate /usr/share/applications/dota2cfgchanger.desktop
    runuser -u builder -- bash /work/project/scripts/check-installed-package.sh
    install -Dm644 /dev/null /home/builder/.config/DotaManager/backups/retained-user-data
    cp "${packages[0]}" PKGBUILD .SRCINFO packaging-support.patch /work/artifacts/
    pacman -Rns --noconfirm dota2cfgchanger
    [[ ! -e /usr/bin/DotaManager && ! -e /usr/share/applications/dota2cfgchanger.desktop ]]
    [[ -f /home/builder/.config/DotaManager/backups/retained-user-data ]]
    echo 'Arch build, checks, install, launch and uninstall passed'
    ;;
*) exit 2 ;;
esac
