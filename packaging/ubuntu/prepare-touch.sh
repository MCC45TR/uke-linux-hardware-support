#!/bin/bash
# Source-only import: apply the shared geometry/assignment fixes, never run donor code.
reference=$root/uke-sensors/referances/nt36532-thp
while IFS=$'\t' read -r path hash; do
 printf '%s  %s\n' "$hash" "$reference/$path" | sha256sum -c -
done < "$root/packaging/ubuntu/touch-inputs.tsv"
# This inspected project script only copies, filters, patches and checks hashes.
bash "$stage/src/prepare-touch-core.sh" "$reference"
printf '%s\n' 'THP source: https://github.com/ianchb/xiaomi-sheng-thp' 'Revision: 34046210932d654a4c0df0121ecc31c008f8148c' 'License: Apache-2.0; device-private inputs excluded' > "$stage/THP-ORIGIN"
