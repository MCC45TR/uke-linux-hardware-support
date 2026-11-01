#!/bin/bash
# Generate snapshot metadata from the canonical release overlay. No upstream
# code execution, branch creation, key handling or upload occurs here.
set -euo pipefail
umask 022
root=$(git -C "$(dirname -- "$0")" rev-parse --show-toplevel)
package=${1:?Usage: prepare-main-overlay.sh PACKAGE NEW_OUTPUT_DIRECTORY}
output=${2:?new output directory required}
[[ $package == uke-speaker-firmware ]] || { echo 'No qualified main snapshot adapter for this package' >&2; exit 78; }
[[ ! -e $output ]]
canonical=$root/packaging/ubuntu/recipes/$package/debian
version=$(dpkg-parsechangelog --file "$canonical/changelog" -SVersion)
[[ $version == 3.0.304.0-0ubuntu*~tenacious1 ]]
snapshot=${version/-0ubuntu/+uke0ubuntu}
mkdir -p "$output"
cp -a "$canonical/." "$output/"
cp "$root/xiaomi-uke-firmware/src/audit-speaker-source.sh" "$output/audit-speaker-source"
printf '3.0 (native)\n' > "$output/source/format"
sed "1s/($version)/($snapshot)/" "$canonical/changelog" > "$output/changelog"
sed '/^Files:/,$d' "$canonical/copyright" > "$output/copyright"
cat >> "$output/copyright" <<'COPYRIGHT'
Files: *
Copyright: 2026 MCC45TR
License: MIT
COPYRIGHT
sed -e 's/^/ /' -e 's/^ $/ ./' "$root/LICENSE" >> "$output/copyright"
cat >> "$output/copyright" <<'COPYRIGHT'

Files: source/fs19xx.fsm source/odm-NOTICE.xml.gz metadata/LICENSE.Apache-2.0
Copyright: Original OEM contributors identified by the bundled ODM notice
License: Apache-2.0
 The complete text is retained in metadata/LICENSE.Apache-2.0 and installed
 with the original notice. This grant is specific to the mapped preset.
COPYRIGHT
