#!/usr/bin/bash
# COPR only: retain the214 source cohort and apply two scoped runtime fixes.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
boot=$(realpath "${1:?Prepared boot source}")
curve=$(realpath "${2:?Prepared brightness source}")
overlay=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
[[ -f $boot/src/critical-health.hpp && -f $curve/src/oss-brightness-curve.hpp ]]
mkdir -p "$work/inverse"
for path in src/slot-control.hpp src/userdata-update.hpp tests/slot-control.cpp;do
 install -Dm644 "$boot/$path" "$work/inverse/$path"
done
git -C "$boot" apply --check --whitespace=error "$overlay/packaging/accept-snapshot-identity.patch"
git -C "$boot" apply --whitespace=error "$overlay/packaging/accept-snapshot-identity.patch"
mkdir "$work/undo"
cp -a "$boot/src" "$boot/tests" "$work/undo/"
git -C "$work/undo" apply -R --whitespace=error "$overlay/packaging/accept-snapshot-identity.patch"
for path in src/slot-control.hpp src/userdata-update.hpp tests/slot-control.cpp;do
 cmp "$work/inverse/$path" "$work/undo/$path"
done
# The startup helper's numeric OSS/LUT table remains the inherited214 source.
for path in src/brightness-curve.cpp src/brightness-curve-config.hpp tests/brightness-curve.cpp packaging/30-uke-brightness-curve.sh docs/BRIGHTNESS-CURVE.md;do
 install -Dm644 "$overlay/$path" "$curve/$path"
done
sh -n "$curve/packaging/30-uke-brightness-curve.sh"
printf 'Control session fixes prepared: full observed GPT identity, curve-only merge, retained compositor/CRC/protection gates.\n'
