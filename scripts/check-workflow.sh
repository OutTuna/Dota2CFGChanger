#!/usr/bin/env bash

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
