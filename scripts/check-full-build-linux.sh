#!/usr/bin/env bash

set -euo pipefail
cd "$(dirname "$0")/.."

if [ "${1:-}" = "--install-deps" ]; then
  echo "Installing build dependencies via apt-get ..."
  apt-get update -qq
  apt-get install -y -qq cmake build-essential libgl1-mesa-dev xorg-dev libssl-dev
fi

for tool in cmake g++ pkg-config; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "❌ Missing required tool: $tool"
    echo "   Re-run with --install-deps (as root), or install it yourself."
    exit 1
  fi
done

BUILD_DIR="build-linux-check"
echo "Configuring into ./$BUILD_DIR (kept around so repeat runs reuse FetchContent downloads) ..."
cmake -S . -B "$BUILD_DIR"

echo "Building (this pulls + builds glfw/curl/cpr/imgui the first time, so it's slow once, fast after) ..."
cmake --build "$BUILD_DIR" --config Release -j"$(nproc)"

if [ -x "$BUILD_DIR/DotaManager" ]; then
  echo "✅ Built successfully: $BUILD_DIR/DotaManager"
  file "$BUILD_DIR/DotaManager" 2>/dev/null || true
else
  echo "❌ Build finished but the expected binary wasn't found at $BUILD_DIR/DotaManager"
  exit 1
fi
