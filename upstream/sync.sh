#!/bin/sh
# Shallow-clone pinned upstreams into $1 (default: ./upstream-src).
# Verifies SHAs after clone. No patches applied (see docs/SPIKE.md).
set -eu
DEST="${1:-./upstream-src}"
# shellcheck disable=SC1091
. "$(dirname "$0")/pins.env"
mkdir -p "$DEST"
clone_pin() {
    _name="$1"; _url="$2"; _ref="$3"; _sha="$4"
    if [ -d "$DEST/$_name" ]; then
        echo "$_name already present at $DEST/$_name (skipping)"
        return
    fi
    git clone --depth 1 --branch "$_ref" "$_url" "$DEST/$_name"
    _got="$(git -C "$DEST/$_name" rev-parse HEAD)"
    if [ "$_got" != "$_sha" ]; then
        echo "WARNING: $_name HEAD $_got != pinned $_sha (branch moved) — spike was verified at the pinned SHA" >&2
    else
        echo "$_name pinned OK ($_sha)"
    fi
}
clone_pin mixxx "$MIXXX_URL" "$MIXXX_BRANCH" "$MIXXX_SHA"
clone_pin lmms "$LMMS_URL" "$LMMS_TAG" "$LMMS_SHA"
echo "upstreams ready in $DEST"
