#!/bin/bash
# Generate snapshot metadata from the canonical release overlay. No upstream
# code execution, branch creation, key handling or upload occurs here.
set -euo pipefail
umask 022
root=$(git -C "$(dirname -- "$0")" rev-parse --show-toplevel)
package=${1:?Usage: prepare-main-overlay.sh PACKAGE NEW_OUTPUT_DIRECTORY}
output=${2:?new output directory required}
[[ ! -e $output ]]
canonical=$root/packaging/ubuntu/recipes/$package/debian
case $package in
 libssc)
  version=$(dpkg-parsechangelog --file "$canonical/changelog" -SVersion)
  [[ $version == 0.4.4-2ubuntu1~uke1~tenacious1 ]]
  snapshot=${version/-/+uke}
  mkdir -p "$output"
  cp -a "$canonical/." "$output/"
  cp "$root/packaging/ubuntu/recipes/uke-speaker-firmware/debian/verify-main-inputs" "$output/verify-main-inputs"
  cp "$root/packaging/ubuntu/prepare-main-libssc.sh" "$output/prepare-libssc"
  printf '3.0 (native)\n' > "$output/source/format"
  sed "1s/($version)/($snapshot)/" "$canonical/changelog" > "$output/changelog"
  sed -e 's|dh $@ --buildsystem=meson|dh $@ --buildsystem=meson --sourcedirectory=upstream|' \
      -e '/^override_dh_auto_configure:/a\	 bash debian/verify-main-inputs\n\t bash debian/prepare-libssc' \
      -e 's|dh_auto_configure -- |dh_auto_configure --sourcedirectory=upstream -- |' "$canonical/rules" > "$output/rules"
  cat >> "$output/rules" <<'RULES'

override_dh_auto_clean:
	bash debian/verify-main-inputs
	if test -d upstream; then dh_auto_clean --sourcedirectory=upstream; fi
	rm -rf upstream
RULES
  cat >> "$output/copyright" <<'COPYRIGHT'

Files: AGENTS.md README.md LICENSE debian/*
Copyright: 2026 MCC45TR
License: MIT
COPYRIGHT
  exit 0;;
 uke-bluetooth|uke-speaker-orientation|uke-core-meta|uke-front-color|uke-proximity-capture|uke-installation-profile)
  version=$(dpkg-parsechangelog --file "$canonical/changelog" -SVersion)
  [[ $version == 0.1.0-0ubuntu1~tenacious1 || ( $package == uke-core-meta && $version == 1.0.7-0ubuntu1~tenacious1 ) || ( $package == uke-installation-profile && $version == 1.0.31-0ubuntu1~tenacious1 ) ]]
  snapshot=${version/-0ubuntu/+uke0ubuntu}
  mkdir -p "$output"
  cp -a "$canonical/." "$output/"
  cp "$root/packaging/ubuntu/recipes/uke-speaker-firmware/debian/verify-main-inputs" "$output/verify-main-inputs"
  printf '3.0 (native)\n' > "$output/source/format"
  sed "1s/($version)/($snapshot)/" "$canonical/changelog" > "$output/changelog"
  sed -e '/^override_dh_auto_configure:/a\	 bash debian/verify-main-inputs' \
      -e '/^override_dh_auto_clean:/a\	 bash debian/verify-main-inputs' \
      -e 's| src/| source/src/|g' -e 's| packaging/| source/packaging/|g' "$canonical/rules" > "$output/rules"
  if [[ $package == uke-front-color ]]; then
   sed -i -e 's|dh_auto_build -- build/uke-front-color|dh_auto_build --sourcedirectory=source -- build/uke-front-color|' \
       -e 's|install -Dm755 build/uke-front-color|install -Dm755 source/build/uke-front-color|' \
       -e '/rm -rf build$/s|$| source/build|' "$output/rules"
  fi
  for list in "$output/"*.install; do
   [[ -f $list ]] || continue
   sed -i -E 's#^(src/|packaging/)#source/\1#' "$list"
  done
  cat >> "$output/copyright" <<'COPYRIGHT'

Files: AGENTS.md README.md LICENSE debian/*
Copyright: 2026 MCC45TR
License: MIT
COPYRIGHT
  exit 0;;
 uke-displayport-audio|uke-speaker-controls|uke-gpu-defaults)
  version=$(dpkg-parsechangelog --file "$canonical/changelog" -SVersion)
  [[ $version == 0.1.0-0ubuntu1~tenacious1 ]]
  snapshot=${version/-0ubuntu/+uke0ubuntu}
  mkdir -p "$output"
  cp -a "$canonical/." "$output/"
  cp "$root/packaging/ubuntu/recipes/uke-speaker-firmware/debian/verify-main-inputs" "$output/verify-main-inputs"
  printf '3.0 (native)\n' > "$output/source/format"
  sed "1s/($version)/($snapshot)/" "$canonical/changelog" > "$output/changelog"
  # The common component, including its source license, is nested at source/.
  # Preserve release metadata and original install choices; prefix only inputs.
  sed -e '/^override_dh_auto_configure:/a\	 bash debian/verify-main-inputs' \
      -e '/^override_dh_auto_clean:/a\	 bash debian/verify-main-inputs' \
      -e 's|dh_install$|dh_install --sourcedir=source|' \
      -e 's|install -Dm644 src/|install -Dm644 source/src/|' "$canonical/rules" > "$output/rules"
  if [[ $package == uke-displayport-audio ]]; then
   printf '\noverride_dh_install:\n\tdh_install --sourcedir=source\n' >> "$output/rules"
  fi
  cat >> "$output/copyright" <<'COPYRIGHT'

Files: AGENTS.md README.md LICENSE debian/*
Copyright: 2026 MCC45TR
License: MIT
COPYRIGHT
  exit 0;;
 uke-speaker-firmware) ;;
 *) echo 'No main snapshot adapter for this package' >&2; exit 78;;
esac
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
