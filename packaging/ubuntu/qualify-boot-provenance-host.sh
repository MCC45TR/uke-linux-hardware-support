#!/bin/bash
# Test project-owned metadata code against official ARM64 packages only.
# No Uke source helper, compiler, dracut producer, kernel or device is executed.
set -euo pipefail
export LC_ALL=C DEBIAN_FRONTEND=noninteractive
[[ $(uname -m) == aarch64 && $(dpkg --print-architecture) == arm64 ]]
source /etc/os-release
[[ $VERSION_CODENAME == tenacious ]]
mkdir -p /evidence
printf '#!/bin/sh\nexit 101\n' > /usr/sbin/policy-rc.d
chmod 755 /usr/sbin/policy-rc.d
apt-get update
apt-get install -y --no-install-recommends dracut-core dracut-install f2fs-tools systemd udev binutils
probe=$(mktemp -d)
trap 'rm -rf "$probe"' EXIT
mkdir -p "$probe/work/base/usr/bin" "$probe/work/base/usr/lib/systemd" "$probe/work/base/usr/lib/aarch64-linux-gnu" "$probe/work/base/usr/lib/dracut/hooks" "$probe/work/base/etc" "$probe/source/dracut" "$probe/source/licenses"
for command in cat readlink; do cp -L "/usr/bin/$command" "$probe/work/base/usr/bin/$command"; done
cp -L /usr/lib/systemd/systemd "$probe/work/base/usr/lib/systemd/systemd"
cp -L /usr/lib/aarch64-linux-gnu/libc.so.6 "$probe/work/base/usr/lib/aarch64-linux-gnu/libc.so.6"
hook=$(find /usr/lib/dracut/modules.d -type f -name '*.sh' -print -quit)
[[ -n $hook ]]
cp "$hook" "$probe/work/base/usr/lib/dracut/hooks/relocated-fixture.sh"
echo 'NAME=HostMetadataFixture' > "$probe/work/base/etc/initrd-release"
bash /repo/packaging/ubuntu/recipes/uke-linux-kernel/debian/boot-adapter/ubuntu-dpkg-provenance.sh "$probe/work" "$probe/source"
while IFS= read -r -d '' file; do
 if [[ $(od -An -tx1 -N4 "$file" | tr -d ' \n') == 7f454c46 ]]; then
  readelf -h "$file" | grep -E 'Machine:.*AArch64'
 fi
done < <(find "$probe/work/base" -type f -print0)
cp "$probe/work/payload-provenance.tsv" "$probe/work/build-debs.tsv" /evidence/
(cd "$probe/work"; find licenses -type f -print0 | sort -z | xargs -0 sha256sum) > /evidence/license-files.sha256
printf '%s\n' 'Native ARM64 metadata backend passed with official Tenacious package files.' 'Only copied stock files were inspected. No Uke source or staged executable ran.' 'This is not a constructed or boot-qualified initramfs.' > /evidence/qualification.txt
