#!/bin/bash
# Runs inside a disposable, native ARM64 Tenacious server container.
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive LC_ALL=C
package=${1:?package required}
[[ $(uname -m) == aarch64 && $(dpkg --print-architecture) == arm64 ]]
source /etc/os-release
[[ $VERSION_CODENAME == tenacious && $VERSION_ID == 27.04 ]]
cd /repo
awk -F '\t' -v p="$package" '$1==p {found=1} END {exit !found}' packaging/ubuntu/components.tsv
printf '#!/bin/sh\nexit 101\n' > /usr/sbin/policy-rc.d
chmod 755 /usr/sbin/policy-rc.d
apt-get update
apt-get install -y --no-install-recommends build-essential dpkg-dev debhelper curl ca-certificates git jq xz-utils libarchive-tools gnupg gpgv
git config --global --add safe.directory /repo
useradd --create-home builder
mkdir -p /work /evidence
chown builder:builder /work
cp /etc/os-release /evidence/os-release
cp /var/lib/apt/lists/*InRelease /evidence/
git rev-parse HEAD > /evidence/packaging-commit
build_one() {
 local pkg=$1 out=/work/$1
 bash packaging/ubuntu/fetch-upstreams.sh "$pkg"
 if [[ $pkg == uke-orangefox-recovery ]]; then bash packaging/ubuntu/fetch-recovery.sh; fi
 bash packaging/ubuntu/prepare-source.sh "$pkg" "$out"
 local stage
 stage=$(find "$out" -mindepth 1 -maxdepth 1 -type d -name "$pkg-*" -print)
 [[ -n $stage && $stage != *$'\n'* ]]
 local dependencies
 dependencies=$(sed -n 's/^Build-Depends: //p' "$stage/debian/control")
 [[ -n $dependencies ]]
 apt-get satisfy -y --no-install-recommends "$dependencies"
 dpkg-query -W > "/evidence/$pkg-build-environment.tsv"
 chown -R builder:builder "$out"
 # Preserve failure logs but never archive a build tree or its ephemeral keys.
 set +e
 runuser -u builder -- bash -c 'cd "$1"; dpkg-buildpackage -b -us -uc' bash "$stage" 2>&1 | tee "/evidence/$pkg-build.log"
 local status=${PIPESTATUS[0]}
 set -e
 find "$out" -maxdepth 1 -type f \( -name '*.deb' -o -name '*.dsc' -o -name '*.changes' -o -name '*.buildinfo' -o -name '*.tar.xz' -o -name SHA256SUMS \) -exec cp -t /evidence/ -- {} +
 printf '%s\t%s\n' "$pkg" "$status" >> /evidence/build-status.tsv
 ((status == 0)) || return "$status"
 for deb in "$out"/*.deb; do
  [[ $(dpkg-deb -f "$deb" Architecture) == arm64 || $(dpkg-deb -f "$deb" Architecture) == all ]]
  dpkg-deb -c "$deb" >> "/evidence/$pkg-payload.txt"
 done
}
if [[ $package == iio-sensor-proxy ]]; then
 build_one libssc
 apt-get install -y /work/libssc/*.deb
fi
build_one "$package"
if [[ $package == uke-speaker-firmware ]]; then
 # No maintainer hooks or device services exist in this data-only package.
 apt-get install -y /work/uke-speaker-firmware/*.deb
 preset=/usr/lib/firmware/qcom/uke/audio/fs19xx.fsm
 printf '%s  %s\n' f319e71d60ed19d539296665fa06cc07fbcf93bf82f773c787bed65289129e7b "$preset" | sha256sum -c -
 printf '\nlocal-conffile-test\n' >> "$preset"
 cp "$preset" /work/expected-conffile
 dpkg --force-confold -i /work/uke-speaker-firmware/*.deb
 cmp /work/expected-conffile "$preset"
 apt-get remove -y uke-speaker-firmware
 cmp /work/expected-conffile "$preset"
 apt-get purge -y uke-speaker-firmware
 [[ ! -e $preset ]]
 # Official git-build-recipe is an unavoidable Launchpad recipe host tool.
 # Its upstream Python dependencies remain confined to this server container.
 apt-get install -y git-build-recipe devscripts pristine-tar quilt
 dpkg-query -W git-build-recipe > /evidence/recipe-tool-version.tsv
 dpkg-source -x /work/uke-speaker-firmware/*.dsc /work/recipe-pristine
 bash packaging/ubuntu/export-recipe-source.sh /work/recipe-pristine /work/recipe-source
 chown -R builder:builder /work/recipe-source
 printf '%s\n' '# git-build-recipe format 0.4 deb-version {debupstream}-0ubuntu1~tenacious1+git{revtime}' '/work/recipe-source launchpad/uke-speaker-firmware' > /work/test.recipe
 runuser -u builder -- git-build-recipe --distribution tenacious /work/test.recipe /work/recipe-result
 find /work/recipe-result -maxdepth 1 -type f \( -name '*.dsc' -o -name '*.tar.*' -o -name '*.changes' \) -exec cp -t /evidence/ -- {} +
 echo 'install/reinstall/local-conffile/remove/purge and recipe source construction passed' > /evidence/firmware-acceptance.txt
fi
(cd /evidence; find . -maxdepth 1 -type f ! -name ARTIFACT-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > ARTIFACT-SHA256SUMS)
