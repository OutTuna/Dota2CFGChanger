#!/usr/bin/env bash
# Computes the next vMAJOR.MINOR tag for this repo.
#
# Version scheme: vMAJOR.MINOR (e.g. v1.4). A normal run bumps MINOR
# (v1.4 -> v1.5). A MAJOR bump (v1.4 -> v2.0, MINOR resets to 0) happens
# when any commit since the last tag:
#   - contains a "BREAKING CHANGE:" footer (conventional commits), or
#   - has a conventional-commit "!" marker, e.g. "feat!: ..." / "fix(ui)!: ...", or
#   - contains the literal marker "[major]" anywhere in the message
# or when FORCE_MAJOR=true is set in the environment (this is what the
# workflow_dispatch "force_major" input maps to in CI).
# If no vX.Y tag exists yet in the repo, this starts fresh at v1.0.
#
# Usage:
#   ./scripts/compute-next-version.sh              # dry run, prints to stdout
#   FORCE_MAJOR=true ./scripts/compute-next-version.sh
#   GITHUB_OUTPUT=/path/to/file ./scripts/compute-next-version.sh
#     (as used from CI: writes version=... / is_major=... there instead)
#
# This script is deliberately the single source of truth for the version
# logic -- both the CI workflow and a developer running it locally call
# the exact same code, so "does the next version look right" is something
# you can check with one local command instead of pushing and watching
# Actions.

set -euo pipefail

FORCE_MAJOR="${FORCE_MAJOR:-false}"

git fetch --tags --force --quiet || true

LAST_TAG=$(git tag --list 'v[0-9]*.[0-9]*' | sort -V | tail -n1)

if [ -z "$LAST_TAG" ]; then
  NEW_VERSION="v1.0"
  IS_MAJOR="false"
  echo "No existing vX.Y tag found -- starting at $NEW_VERSION" >&2
else
  echo "Last tag: $LAST_TAG" >&2
  COMMITS=$(git log "${LAST_TAG}..HEAD" --pretty=%B || true)

  MAJOR_BUMP=false
  if [ "$FORCE_MAJOR" = "true" ]; then
    MAJOR_BUMP=true
    echo "Major bump forced (FORCE_MAJOR=true)" >&2
  fi
  if echo "$COMMITS" | grep -qi 'BREAKING CHANGE'; then
    MAJOR_BUMP=true
    echo "Major bump: found 'BREAKING CHANGE' footer" >&2
  fi
  if echo "$COMMITS" | grep -qi '\[major\]'; then
    MAJOR_BUMP=true
    echo "Major bump: found '[major]' marker" >&2
  fi
  if echo "$COMMITS" | grep -Eq '^[a-zA-Z]+(\([a-zA-Z0-9_-]+\))?!:'; then
    MAJOR_BUMP=true
    echo "Major bump: found conventional-commit '!' breaking-change marker" >&2
  fi

  VER="${LAST_TAG#v}"
  MAJOR="${VER%%.*}"
  MINOR="${VER##*.}"

  if [ "$MAJOR_BUMP" = true ]; then
    NEW_VERSION="v$((MAJOR + 1)).0"
    IS_MAJOR="true"
  else
    NEW_VERSION="v${MAJOR}.$((MINOR + 1))"
    IS_MAJOR="false"
  fi
fi

echo "Next version: $NEW_VERSION (major bump: $IS_MAJOR)" >&2

if [ -n "${GITHUB_OUTPUT:-}" ]; then
  echo "version=$NEW_VERSION" >> "$GITHUB_OUTPUT"
  echo "is_major=$IS_MAJOR"   >> "$GITHUB_OUTPUT"
else
  # Local/dry-run mode: print machine-readable result to stdout so it can
  # also be piped/parsed if needed, on top of the human-readable log lines
  # above (which go to stderr, so they don't get mixed into stdout output).
  echo "version=$NEW_VERSION"
  echo "is_major=$IS_MAJOR"
fi
