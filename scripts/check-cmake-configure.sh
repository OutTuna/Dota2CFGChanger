#!/usr/bin/env bash

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
