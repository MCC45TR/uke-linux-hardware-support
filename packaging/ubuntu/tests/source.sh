#!/bin/bash
# Static source-contract checks only; never build or run device code.
set -euo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
jq -e 'length == 43 and (map(.name)|unique|length)==43 and all(.[]; .status != "ready")' "$root/packaging/ubuntu/coverage.json" >/dev/null
for package in uke-battery-observer uke-front-color; do
 bash "$root/packaging/ubuntu/prepare-source.sh" "$package" "$work/$package"
 dsc=("$work/$package/"*.dsc)
 dpkg-source --no-check -x "${dsc[0]}" "$work/$package-extracted"
 debian=$work/$package-extracted/debian
 [[ $(dpkg-parsechangelog -l"$debian/changelog" -S Distribution) == tenacious ]]
 grep -qx 'Architecture: arm64' "$debian/control"
 grep -q 'dh_installsystemd --no-enable --no-start' "$debian/rules"
 ! grep -Eq 'dnf|rpm|dracut|selinux|python' "$debian/control"
 find "$debian" -type f -name '*.postinst' -exec sh -n {} \;
 # Maintainer scripts must not change boot selection or activate hardware.
 ! grep -ER 'efibootmgr|flashrom|fastboot|update-initramfs|systemctl (start|enable)' "$debian"
 (cd "$work/$package"; sha256sum -c SHA256SUMS)
done
# Unknown sources and accidental output reuse fail closed.
if bash "$root/packaging/ubuntu/prepare-source.sh" unknown "$work/unknown" 2>/dev/null; then exit 1; fi
if bash "$root/packaging/ubuntu/prepare-source.sh" uke-front-color "$work/uke-front-color" 2>/dev/null; then exit 1; fi
printf '%s\n' 'PASS: two source archives, extraction, metadata, activation policy, checksums and rejection cases'
