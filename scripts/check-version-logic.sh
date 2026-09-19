#!/usr/bin/env bash
# Test-point #4: does the version-bump logic itself do the right thing.
#
# Builds a disposable, throwaway git repo (NOT your real one -- this
# never touches the repo you run it from) with a few scripted commit
# histories, and checks that scripts/compute-next-version.sh bumps
# minor/major exactly when expected. Run this after touching
# scripts/compute-next-version.sh, or any time you're not sure whether a
# given commit message would trigger a major bump.
#
# Usage: ./scripts/check-version-logic.sh

set -euo pipefail
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
COMPUTE_SCRIPT="$REPO_ROOT/scripts/compute-next-version.sh"

SANDBOX="$(mktemp -d)"
trap 'rm -rf "$SANDBOX"' EXIT

fail=0

run_case() {
  local desc="$1" expected_version="$2"; shift 2
  local commit_messages=("$@")

  local repo="$SANDBOX/$(echo "$desc" | tr -c 'a-zA-Z0-9' '_')"
  git init -q "$repo"
  (
    cd "$repo"
    git config user.name "test"
    git config user.email "test@example.com"
    echo "init" > file.txt && git add file.txt && git commit -qm "chore: init"
    git tag v1.4

    for msg in "${commit_messages[@]}"; do
      echo "$msg" >> file.txt
      git add file.txt
      git commit -qm "$msg"
    done
  )

  local output version
  output=$(cd "$repo" && bash "$COMPUTE_SCRIPT" 2>/dev/null)
  version=$(echo "$output" | grep '^version=' | cut -d= -f2)

  if [ "$version" = "$expected_version" ]; then
    echo "✅ $desc -> $version"
  else
    echo "❌ $desc -> got $version, expected $expected_version"
    fail=1
  fi
}

echo "Baseline tag for every case below: v1.4"
echo

run_case "no special markers (plain commits)" "v1.5" \
  "fix: adjust padding" \
  "chore: tidy comments"

run_case "BREAKING CHANGE footer" "v2.0" \
  "feat: new settings format

BREAKING CHANGE: settings.json schema changed, old files are ignored"

run_case "conventional-commit ! marker" "v2.0" \
  "feat!: drop support for the old registry-based settings"

run_case "scoped conventional-commit ! marker" "v2.0" \
  "fix(ui)!: rework the confirm-copy popup contract"

run_case "[major] literal marker" "v2.0" \
  "big rewrite of the avatar cache [major]"

# The "no tag exists yet" case needs its own repo with no v1.4 baseline
# tag at all, so it's set up by hand instead of via run_case.
repo="$SANDBOX/no_tag_yet"
git init -q "$repo"
(
  cd "$repo"
  git config user.name "test"
  git config user.email "test@example.com"
  echo "init" > file.txt && git add file.txt && git commit -qm "chore: init"
)
output=$(cd "$repo" && bash "$COMPUTE_SCRIPT" 2>/dev/null)
version=$(echo "$output" | grep '^version=' | cut -d= -f2)
if [ "$version" = "v1.0" ]; then
  echo "✅ no tag exists yet -> $version"
else
  echo "❌ no tag exists yet -> got $version, expected v1.0"
  fail=1
fi

echo
if [ "$fail" = "0" ]; then
  echo "✅ All version-logic cases passed."
else
  echo "❌ Some version-logic cases failed (see above)."
  exit 1
fi
