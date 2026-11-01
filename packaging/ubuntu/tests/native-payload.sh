#!/bin/bash
set -euo pipefail
umask 022
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
mkdir -p "$scratch/package/DEBIAN" "$scratch/package/usr/share/uke-fixture"
printf '%s\n' 'Package: uke-payload-fixture' 'Version: 1' 'Architecture: arm64' 'Maintainer: Test <test@example.invalid>' 'Description: isolated data fixture' > "$scratch/package/DEBIAN/control"
echo data > "$scratch/package/usr/share/uke-fixture/data"
check() {
 dpkg-deb --build --root-owner-group "$scratch/package" "$scratch/test.deb" >/dev/null || return
 bash "$root/packaging/ubuntu/inspect-native-deb.sh" "$scratch/test.deb" > "$scratch/result" 2>&1
}
check
echo 'print("must never execute")' > "$scratch/package/usr/share/uke-fixture/test.py"
if check; then echo 'Python payload was admitted' >&2; exit 1; fi
grep -F 'Python payload is not admitted' "$scratch/result"
rm "$scratch/package/usr/share/uke-fixture/test.py"
echo 'Depends: python3' >> "$scratch/package/DEBIAN/control"
if check; then echo 'Python runtime dependency was admitted' >&2; exit 1; fi
grep -F 'Python runtime dependency is not admitted' "$scratch/result"
sed -i '/^Depends:/d; s/^Architecture: arm64$/Architecture: amd64/' "$scratch/package/DEBIAN/control"
if check; then echo 'Wrong package architecture was admitted' >&2; exit 1; fi
echo 'Native payload refusal fixtures passed; no packaged code executed.'
