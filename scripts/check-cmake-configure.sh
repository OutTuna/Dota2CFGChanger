#!/usr/bin/env bash
# Test-point #2: does `cmake -S . -B build` even configure cleanly.
#
# This will NOT catch everything the Windows build in CI does (MSVC-
# specific code, the .rc icon step, an actual link), but it catches a
# large class of CMakeLists.txt mistakes in seconds, on any OS, without
# needing to push and wait for the windows-latest runner: unknown/typo'd
# CMake options, missing FetchContent sources, wrong target names, a
# forced boolean cache variable that contradicts another one, etc. -- the
# same category of bug as the CURL_USE_OPENSSL / CURL_USE_SCHANNEL
# regression from the previous patch, which this exact check would have
# caught immediately, locally, instead of failing silently at runtime.
#
# For a deeper check (does it actually compile and link), see
# check-full-build-linux.sh -- slower, and only proves the Linux/
# non-Windows code paths, but it's the closest thing to a real build you
# can run without a Windows machine.
#
# Usage: ./scripts/check-cmake-configure.sh

set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR="$(mktemp -d)"
trap 'rm -rf "$BUILD_DIR"' EXIT

echo "Configuring into a throwaway build dir: $BUILD_DIR"
if cmake -S . -B "$BUILD_DIR" > "$BUILD_DIR/configure.log" 2>&1; then
  echo "✅ CMake configure succeeded."
else
  echo "❌ CMake configure failed. Log:"
  echo "----------------------------------------"
  cat "$BUILD_DIR/configure.log"
  echo "----------------------------------------"
  exit 1
fi
