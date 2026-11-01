#!/bin/bash
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
host=$work/host
mkdir -p "$host/usr/lib/udev/hwdb.d" "$host/var/lib/dpkg/info"
printf 'mouse:*\n ID_INPUT=1\n\n' > "$host/usr/lib/udev/hwdb.d/99-fixture.hwdb"
printf 'Package: hwdb-fixture\nStatus: install ok installed\nArchitecture: all\nVersion: 1.0\nMaintainer: Test <test@example.invalid>\nDescription: isolated data fixture\n\n' > "$host/var/lib/dpkg/status"
echo 1 > "$host/var/lib/dpkg/info/format"
echo /usr/lib/udev/hwdb.d/99-fixture.hwdb > "$host/var/lib/dpkg/info/hwdb-fixture.list"
bash "$here/verify-hwdb.sh" --prepare "$host" "$work/attested"
cp "$host/usr/lib/udev/hwdb.bin" "$work/payload"
chmod u+w "$work/payload"
export UKE_HWDB_RECEIPT=$work/attested
bash "$here/verify-hwdb.sh" "$work/payload" "$host" "$work/receipt.tsv"
grep -F hwdb-fixture "$work/receipt.tsv"
printf 'corruption' >> "$work/payload"
if bash "$here/verify-hwdb.sh" "$work/payload" "$host" "$work/receipt.tsv" >/dev/null 2>&1; then
 echo 'Changed generated cache was admitted' >&2; exit 1
fi
cp "$host/usr/lib/udev/hwdb.bin" "$work/payload"
cp "$host/usr/lib/udev/hwdb.d/99-fixture.hwdb" "$host/usr/lib/udev/hwdb.d/98-unowned.hwdb"
if bash "$here/verify-hwdb.sh" "$work/payload" "$host" "$work/receipt.tsv" >/dev/null 2>&1; then
 echo 'Unowned hardware data input was admitted' >&2; exit 1
fi
echo 'Generated hardware cache verified; changed cache and unowned source refused.'
