#!/usr/bin/env bash
# Test-point #1: validate every .github/workflows/*.y*ml file the way
# GitHub itself does, *before* pushing.
#
# Plain YAML parsers (python -c "import yaml") only catch broken YAML
# syntax. They will NOT catch most of what actually causes GitHub to show
# a workflow run that fails instantly with a red X and ~0s duration --
# that's almost always a *schema*/*expression* problem (bad "on:" block,
# unknown context, wrong input type, typo'd action version, etc.), which
# needs a tool that actually understands the GitHub Actions schema.
# actionlint (https://github.com/rhysd/actionlint) is that tool, and it's
# the same category of checker GitHub's own backend runs before starting
# a job.
#
# Usage: ./scripts/check-workflow.sh
# Exit code: 0 if clean, non-zero if actionlint found anything.

set -euo pipefail
cd "$(dirname "$0")/.."

BIN_DIR="$(mktemp -d)"
trap 'rm -rf "$BIN_DIR"' EXIT

if command -v actionlint >/dev/null 2>&1; then
  ACTIONLINT="actionlint"
else
  echo "actionlint not found on PATH -- downloading a local copy into $BIN_DIR ..."
  curl -sL https://raw.githubusercontent.com/rhysd/actionlint/main/scripts/download-actionlint.bash \
    | bash -s -- latest "$BIN_DIR" >/dev/null
  ACTIONLINT="$BIN_DIR/actionlint"
fi

echo "Running actionlint against .github/workflows/ ..."
if "$ACTIONLINT" -color .github/workflows/*.y*ml; then
  echo "✅ actionlint: no problems found."
else
  echo "❌ actionlint found problems (see above). Fix them before pushing."
  exit 1
fi
