#!/usr/bin/env bash
# Runs every local test-point in order, fast-to-slow, and prints a
# summary. Meant to replace "push -> wait for Actions -> read log ->
# fix -> push again" with something you can run on your own machine.
#
#   ./scripts/run-all-checks.sh            # everything except the full build
#   ./scripts/run-all-checks.sh --full      # also do the full Linux compile+link
#
# Individual checks (see comments in each file for what they catch and
# why):
#   scripts/check-workflow.sh          - actionlint on .github/workflows/
#   scripts/check-version-logic.sh     - unit-tests the version bump rules
#   scripts/check-cmake-configure.sh   - does `cmake configure` succeed
#   scripts/check-full-build-linux.sh  - full compile+link (slow, opt-in)

set -uo pipefail
cd "$(dirname "$0")/.."

RUN_FULL_BUILD=false
if [ "${1:-}" = "--full" ]; then
  RUN_FULL_BUILD=true
fi

declare -a NAMES=()
declare -a RESULTS=()

run_step() {
  local name="$1" script="$2"; shift 2
  echo
  echo "=== $name ==="
  if bash "$script" "$@"; then
    NAMES+=("$name"); RESULTS+=("PASS")
  else
    NAMES+=("$name"); RESULTS+=("FAIL")
  fi
}

run_step "Workflow lint (actionlint)"        scripts/check-workflow.sh
run_step "Version-bump logic (unit tests)"   scripts/check-version-logic.sh
run_step "CMake configure smoke test"        scripts/check-cmake-configure.sh

if [ "$RUN_FULL_BUILD" = true ]; then
  run_step "Full Linux build (compile+link)" scripts/check-full-build-linux.sh
else
  echo
  echo "=== Full Linux build (compile+link) ==="
  echo "Skipped (pass --full to run it -- it's slow the first time)."
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
  echo "All checks passed -- safe to push."
else
  echo "Fix the failing check(s) above before pushing."
fi

exit "$overall"
