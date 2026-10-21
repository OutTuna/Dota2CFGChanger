#!/usr/bin/env bash

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

  echo "version=$NEW_VERSION"
  echo "is_major=$IS_MAJOR"
fi
