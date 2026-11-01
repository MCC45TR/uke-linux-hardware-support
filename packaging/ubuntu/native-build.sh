#!/bin/bash
# Runs inside a disposable, native ARM64 Tenacious server container.
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive LC_ALL=C
package=${1:?package required}
[[ $(uname -m) == aarch64 && $(dpkg --print-architecture) == arm64 ]]
source /etc/os-release
[[ $VERSION_CODENAME == tenacious && $VERSION_ID == 27.04 ]]
cd /repo
git config --global --add safe.directory /repo
awk -F '\t' -v p="$package" '$1==p {found=1} END {exit !found}' packaging/ubuntu/components.tsv
printf '#!/bin/sh\nexit 101\n' > /usr/sbin/policy-rc.d
chmod 755 /usr/sbin/policy-rc.d
apt-get update
apt-get install -y --no-install-recommends build-essential dpkg-dev debhelper curl ca-certificates git jq xz-utils libarchive-tools gnupg gpgv
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
(cd /evidence; find . -maxdepth 1 -type f ! -name ARTIFACT-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > ARTIFACT-SHA256SUMS)
