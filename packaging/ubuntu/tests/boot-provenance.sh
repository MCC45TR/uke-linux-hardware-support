#!/bin/bash
# Metadata fixtures only; no staged executable, kernel helper or device runs.
set -euo pipefail
umask 022
repo=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
adapter=${UKE_BOOT_PROVENANCE_ADAPTER:-$repo/packaging/ubuntu/recipes/uke-linux-kernel/debian/boot-adapter/ubuntu-dpkg-provenance.sh}
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
host=$scratch/host
work=$scratch/work
recipe=$scratch/recipe
mkdir -p "$host/var/lib/dpkg/info" "$host/usr/bin" "$host/usr/lib/dracut/modules.d/90fixture" "$host/usr/share/common-licenses" "$work/base/usr/bin" "$work/base/usr/lib/dracut/hooks" "$work/base/etc" "$recipe/dracut" "$recipe/licenses"
ln -s usr/bin "$host/bin"
printf '#!/bin/sh\n# package-owned fixture, never executed\n' > "$host/usr/bin/fixture-command"
printf '#!/bin/sh\n# relocated package script, never executed\n' > "$host/usr/lib/dracut/modules.d/90fixture/hook.sh"
printf '#!/bin/sh\n# shared project fixture, never executed\n' > "$recipe/dracut/root-guard.sh"
chmod 755 "$host/usr/bin/fixture-command" "$host/usr/lib/dracut/modules.d/90fixture/hook.sh" "$recipe/dracut/root-guard.sh"
cp "$host/usr/bin/fixture-command" "$work/base/usr/bin/fixture-command"
cp "$host/usr/lib/dracut/modules.d/90fixture/hook.sh" "$work/base/usr/lib/dracut/hooks/fixture.sh"
mkdir -p "$work/base/usr/libexec"
cp "$recipe/dracut/root-guard.sh" "$work/base/usr/libexec/uke-root-guard"
echo 'NAME=Fixture' > "$work/base/etc/initrd-release"
echo 'Fixture common license' > "$host/usr/share/common-licenses/MIT"
echo 'Fixture project license' > "$recipe/licenses/builder-MIT"
: > "$host/var/lib/dpkg/status"
echo 1 > "$host/var/lib/dpkg/info/format"
for package in fixture-runtime dracut-core dracut-install f2fs-tools; do
 arch=all; multiarch=; stem=$package
 if [[ $package == fixture-runtime ]]; then arch=arm64; multiarch='Multi-Arch: same'; stem=$package:arm64; fi
 printf '%s\n' "Package: $package" 'Status: install ok installed' "Architecture: $arch" "Version: 1.0" "Source: $package-source (1.0)" "${multiarch:-Priority: optional}" 'Maintainer: Test <test@example.invalid>' 'Description: isolated metadata fixture' '' >> "$host/var/lib/dpkg/status"
 mkdir -p "$host/usr/share/doc/$package"
 echo 'Fixture package copyright' > "$host/usr/share/doc/$package/copyright"
 printf '/usr/share/doc/%s/copyright\n' "$package" > "$host/var/lib/dpkg/info/$stem.list"
done
echo /bin/fixture-command >> "$host/var/lib/dpkg/info/fixture-runtime:arm64.list"
echo /usr/lib/dracut/modules.d/90fixture/hook.sh >> "$host/var/lib/dpkg/info/dracut-core.list"
dpkg-query --admindir "$host/var/lib/dpkg" -S /bin/fixture-command
bash "$adapter" "$work" "$recipe" "$host"
grep -F $'dpkg-owner\tfixture-runtime:arm64' "$work/payload-provenance.tsv"
grep -F $'dpkg-content-match\tdracut-core' "$work/payload-provenance.tsv"
grep -F $'shared-project-source\troot-guard.sh' "$work/payload-provenance.tsv"
grep -F $'generated-metadata\tdracut-output' "$work/payload-provenance.tsv"
test -s "$work/licenses/fixture-runtime:arm64/copyright"
echo corruption >> "$work/base/usr/bin/fixture-command"
if bash "$adapter" "$work" "$recipe" "$host" > "$scratch/rejected" 2>&1; then echo 'Changed owned bytes admitted' >&2; exit 1; fi
grep -E 'differ|EOF|Unmapped executable initramfs payload' "$scratch/rejected"
cp "$host/usr/bin/fixture-command" "$work/base/usr/bin/fixture-command"
cp /bin/true "$work/base/usr/libexec/unmapped"
chmod 644 "$work/base/usr/libexec/unmapped"
if bash "$adapter" "$work" "$recipe" "$host" > "$scratch/rejected" 2>&1; then echo 'Unknown ELF admitted' >&2; exit 1; fi
grep -F 'Unmapped executable initramfs payload' "$scratch/rejected"
rm "$work/base/usr/libexec/unmapped"
printf '#!/bin/sh\n# unregistered script fixture, never executed\n' > "$work/base/etc/unmapped-script"
chmod 644 "$work/base/etc/unmapped-script"
if bash "$adapter" "$work" "$recipe" "$host" > "$scratch/rejected" 2>&1; then echo 'Unknown interpreter script admitted' >&2; exit 1; fi
grep -F 'Unmapped executable initramfs payload' "$scratch/rejected"
rm "$work/base/etc/unmapped-script"
sed -i 's/Architecture: arm64/Architecture: amd64/' "$host/var/lib/dpkg/status"
mv "$host/var/lib/dpkg/info/fixture-runtime:arm64.list" "$host/var/lib/dpkg/info/fixture-runtime:amd64.list"
if bash "$adapter" "$work" "$recipe" "$host" > "$scratch/rejected" 2>&1; then echo 'Foreign package architecture admitted' >&2; exit 1; fi
grep -F 'Foreign boot userspace package' "$scratch/rejected"
echo 'Boot dpkg provenance fixtures passed; no payload executable ran.'
