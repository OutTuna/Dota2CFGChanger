{
  lib,
  stdenv,
  fetchurl,
  cmake,
  ninja,
  pkg-config,
  makeWrapper,
  curl,
  glfw,
  libglvnd,
  openssl,
  nlohmann_json,
  dejavu_fonts,
  zenity,
  python3,
  desktop-file-utils,
  callPackage,
}:
let
  cprSource = fetchurl {
    url = "https://codeload.github.com/libcpr/cpr/tar.gz/refs/tags/1.11.1";
    name = "cpr-1.11.1.tar.gz";
    sha256 = "e84b8ef348f41072609f53aab05bdaab24bf5916c62d99651dfbeaf282a8e0a2";
  };
  imguiSource = fetchurl {
    url = "https://codeload.github.com/ocornut/imgui/tar.gz/refs/tags/v1.91.6";
    name = "imgui-1.91.6.tar.gz";
    sha256 = "c5fbc5dcab1d46064001c3b84d7a88812985cde7e0e9ced03f5677bec1ba502a";
  };
  stbRevision = "2c980bb59875b0d32144a71867fbdebb2f77cd20";
  stbSource = fetchurl {
    url = "https://codeload.github.com/nothings/stb/tar.gz/${stbRevision}";
    name = "stb-${stbRevision}.tar.gz";
    sha256 = "9a955b1b49a4410088a2e0ee2a9c057c3c907d0c1d75454144cb980aca0ba515";
  };
in
stdenv.mkDerivation (finalAttrs: {
  pname = "dota2cfgchanger";
  version = "1.23";

  src = fetchurl {
    url = "https://codeload.github.com/OutTuna/Dota2CFGChanger/tar.gz/refs/tags/v${finalAttrs.version}";
    name = "dota2cfgchanger-${finalAttrs.version}.tar.gz";
    sha256 = "b47dbd04997468675e2bb43af9537bf9beddae67f0cb3172cb011eec5d7d253b";
  };

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
    makeWrapper
  ];
  buildInputs = [
    curl
    glfw
    libglvnd
    openssl
    nlohmann_json
  ];
  nativeCheckInputs = [ python3 ];
  nativeInstallCheckInputs = [ desktop-file-utils ];

  postUnpack = ''
    mkdir -p "$NIX_BUILD_TOP/vendor"
    tar -xzf ${cprSource} -C "$NIX_BUILD_TOP/vendor"
    tar -xzf ${imguiSource} -C "$NIX_BUILD_TOP/vendor"
    tar -xzf ${stbSource} -C "$NIX_BUILD_TOP/vendor"
  '';

  postPatch = ''
    substituteInPlace src/platform/platform.cpp \
      --replace-fail /usr/share/fonts/truetype/dejavu ${dejavu_fonts}/share/fonts/truetype \
      --replace-fail 'roots.push_back("/usr/share/fonts");' 'roots.push_back("/run/current-system/sw/share/fonts");'
  '';

  cmakeFlags = [
    "-DAPP_VERSION=${finalAttrs.version}"
    "-DDOTAMANAGER_SYSTEM_LIBS=ON"
    "-DDOTAMANAGER_PACKAGE_MANAGED=ON"
    "-DDOTAMANAGER_BUILD_TESTS=ON"
    "-DFETCHCONTENT_FULLY_DISCONNECTED=ON"
  ];
  preConfigure = ''
    cmakeFlagsArray+=(
      "-DFETCHCONTENT_SOURCE_DIR_CPR=$NIX_BUILD_TOP/vendor/cpr-1.11.1"
      "-DFETCHCONTENT_SOURCE_DIR_IMGUI=$NIX_BUILD_TOP/vendor/imgui-1.91.6"
      "-DFETCHCONTENT_SOURCE_DIR_STB=$NIX_BUILD_TOP/vendor/stb-${stbRevision}"
    )
  '';

  doCheck = true;
  checkPhase = ''
    runHook preCheck
    ctest --output-on-failure
    python ../tests/core_checks.py ${lib.getDev nlohmann_json}/include
    runHook postCheck
  '';

  installPhase = ''
    runHook preInstall
    cmake --install . --component Runtime
    install -Dm644 "$NIX_BUILD_TOP/vendor/cpr-1.11.1/LICENSE" "$out/share/licenses/dota2cfgchanger/cpr.LICENSE"
    install -Dm644 "$NIX_BUILD_TOP/vendor/imgui-1.91.6/LICENSE.txt" "$out/share/licenses/dota2cfgchanger/imgui.LICENSE"
    install -Dm644 "$NIX_BUILD_TOP/vendor/stb-${stbRevision}/LICENSE" "$out/share/licenses/dota2cfgchanger/stb.LICENSE"
    wrapProgram "$out/bin/DotaManager" --prefix PATH : ${lib.makeBinPath [ zenity ]}
    runHook postInstall
  '';

  doInstallCheck = true;
  installCheckPhase = ''
    runHook preInstallCheck
    test -x "$out/bin/DotaManager"
    test -f "$out/share/icons/hicolor/32x32/apps/dota2cfgchanger.png"
    test -f ${dejavu_fonts}/share/fonts/truetype/DejaVuSans.ttf
    desktop-file-validate "$out/share/applications/dota2cfgchanger.desktop"
    runHook postInstallCheck
  '';

  passthru.tests.nixos = callPackage ./nixos-test.nix {
    dota2cfgchanger = finalAttrs.finalPackage;
  };

  meta = {
    description = "Copy Dota 2 settings between Steam accounts";
    homepage = "https://github.com/OutTuna/Dota2CFGChanger";
    license = lib.licenses.mit;
    mainProgram = "DotaManager";
    platforms = [ "x86_64-linux" ];
    maintainers = lib.optional (lib.maintainers ? outtuna) lib.maintainers.outtuna;
  };
})
