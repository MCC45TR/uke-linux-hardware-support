#!/usr/bin/env bash
# Host-only: exercise the real XML validator and existing profile checks.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
command -v xmllint >/dev/null || { echo 'XML validation requires xmllint (Ubuntu: libxml2-utils; Fedora: libxml2).' >&2; exit 1; }
fixture=$(mktemp -d)
trap 'rm -rf -- "$fixture"' EXIT
cp -a "$root/uke-desktop-metas" "$fixture/metas"
bash "$fixture/metas/tests/plasma-profile.sh"
policy=$fixture/metas/src/uke-kdeconnect.xml
original=$fixture/original.xml
cp "$policy" "$original"
# Well-formed but unsafe ingress must fail the existing semantic oracle.
sed 's/192.168.0.0\/16/0.0.0.0\/0/' "$original" > "$policy"
if bash "$fixture/metas/tests/plasma-profile.sh" > "$fixture/rejected.log" 2>&1; then
    echo 'Unrestricted IPv4 ingress was accepted' >&2; exit 1
fi
# Malformed XML must fail too, rather than skipping a missing validator.
printf '<policy>\n' > "$policy"
if bash "$fixture/metas/tests/plasma-profile.sh" > "$fixture/rejected.log" 2>&1; then
    echo 'Malformed policy was accepted' >&2; exit 1
fi
printf '%s\n' 'XML prerequisite, valid policy, unsafe ingress and malformed-policy checks passed'
