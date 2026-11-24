#!/usr/bin/env bash

set -euo pipefail

FORCE_MAJOR="${FORCE_MAJOR:-false}"

NAME=$(gh release view latest --json name --jq .name)
if [[ ! "$NAME" =~ ^Latest\ Build\ \(v([0-9]+)\.([0-9]+)\)$ ]]; then
  echo "Cannot read the version from the latest release: $NAME" >&2
  exit 1
fi
MAJOR="${BASH_REMATCH[1]}"
MINOR="${BASH_REMATCH[2]}"

if [ "$FORCE_MAJOR" = "true" ]; then
  NEW_VERSION="v$((MAJOR + 1)).0"
else
  NEW_VERSION="v${MAJOR}.$((MINOR + 1))"
fi

echo "Latest release: v${MAJOR}.${MINOR}, next: $NEW_VERSION" >&2

if [ -n "${GITHUB_OUTPUT:-}" ]; then
  echo "version=$NEW_VERSION" >> "$GITHUB_OUTPUT"
else
  echo "version=$NEW_VERSION"
fi
