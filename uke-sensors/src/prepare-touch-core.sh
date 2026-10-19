#!/usr/bin/env bash
set -euo pipefail
[[ $# == 1 ]] || { echo 'Usage: prepare-touch-core.sh verified-reference-directory' >&2; exit 2; }
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
reference=$(cd -- "$1" && pwd)
cd "$reference"
printf '%s\n' \
 'e6408f4fa609f1ffd1718b3c67effb02d3db09e5d79e24c6c046b692097f4516  src/nvt_touch_core.cpp' \
 '42515bb7b4c138cf1148ff2d23f5fbf2838a98bf870500aa89b6b783e53a0c83  src/nvt_touch_core.hpp' | sha256sum -c -
mkdir -p "$root/src/upstream"
stage=$(mktemp -d "$root/src/upstream/.nt36532-thp.XXXXXX")
trap 'rm -rf "$stage"' EXIT
cp src/nvt_touch_core.hpp LICENSE "$stage/"
# Uke geometry is read from the admitted OEM file; donor coordinates never become fallback values.
awk '/constexpr int kShortAxisMaximum/{skip=1} skip && /^struct Profile/{skip=0} !skip{print}' \
 src/nvt_touch_core.cpp > "$stage/nvt_touch_core.cpp"
patch --batch --fuzz=0 -d "$stage" -p1 < "$root/patches/nt36532-thp-runtime-geometry.patch"
patch --batch --fuzz=0 -d "$stage" -p1 < "$root/patches/nt36532-thp-bounded-assignment.patch"
cd "$stage"
printf '%s\n' \
 'bd61ca3441b26769b89a8341834219f721ba4dfe27e976433380b7142cad293b  nvt_touch_core.cpp' \
 '6a0312308c22c4562205317078f01684153ca9a1f7a425f26ea96e003d59a1c8  nvt_touch_core.hpp' | sha256sum -c -
mkdir -p "$root/src/upstream/nt36532-thp"
cp nvt_touch_core.cpp nvt_touch_core.hpp LICENSE "$root/src/upstream/nt36532-thp/"
