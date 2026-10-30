#!/usr/bin/env bash

set -uo pipefail
cd "$(dirname "$0")/.."

RUN_FULL_BUILD=false
if [ "${1:-}" = "--full" ]; then
  RUN_FULL_BUILD=true
fi
BUILD_DIR="${DOTAMANAGER_CHECK_BUILD_DIR:-build-checks}"
PYTHON="${PYTHON:-python3}"

declare -a NAMES=()
declare -a RESULTS=()

run_step() {
  local name="$1"; shift
  echo
  echo "=== $name ==="
  if "$@"; then
    NAMES+=("$name"); RESULTS+=("PASS")
    return 0
  else
    NAMES+=("$name"); RESULTS+=("FAIL")
    return 1
  fi
}

run_step "Workflow lint (actionlint)" bash scripts/check-workflow.sh
run_step "Version-bump logic" bash scripts/check-version-logic.sh
run_step "AppImage ABI checks" "$PYTHON" tests/appimage_checks.py
run_step "Prerelease isolation" "$PYTHON" tests/prerelease_checks.py
run_step "Arch source coverage" "$PYTHON" tests/aur_source_checks.py

if run_step "Configure regression tests" cmake -S . -B "$BUILD_DIR" -DDOTAMANAGER_BUILD_TESTS=ON &&
   run_step "Build regression tests" cmake --build "$BUILD_DIR" --config Release --target dotamanager_checks --parallel; then
  run_step "C++ regression tests" ctest --test-dir "$BUILD_DIR" --build-config Release --output-on-failure --no-tests=error
  if [ "${OS:-}" != "Windows_NT" ]; then
    run_step "Copy and settings fault checks" "$PYTHON" tests/core_checks.py "${DOTAMANAGER_JSON_INCLUDE:-$BUILD_DIR/_deps/json-src/include}"
  fi
  if [ "$RUN_FULL_BUILD" = true ]; then
    run_step "Application build" cmake --build "$BUILD_DIR" --config Release --target DotaManager --parallel
  fi
fi

echo
echo "==================== Summary ===================="
overall=0
for i in "${!NAMES[@]}"; do
  if [ "${RESULTS[$i]}" = "PASS" ]; then
    printf "✅ %s\n" "${NAMES[$i]}"
  else
    printf "❌ %s\n" "${NAMES[$i]}"
    overall=1
  fi
done
echo "==================================================="

if [ "$overall" = "0" ]; then
  echo "Requested checks passed. Platform-specific GUI and release checks still run in CI."
else
  echo "Fix the failing check(s) above before pushing."
fi

exit "$overall"
