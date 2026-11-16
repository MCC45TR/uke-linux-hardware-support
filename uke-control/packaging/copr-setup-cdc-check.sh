#!/usr/bin/bash
# COPR source worker: collect only public project source, without compilation.
set -Eeuo pipefail
[[ $# == 1 && ${UKE_COPR_SOURCE_WORKER:-} == 1 && $(uname -m) == aarch64 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
result=$(realpath -m "$1")
[[ ! -e $result && ! -L $result ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir -p "$result" "$work/setup-cdc/"{src,tests}
for path in src/setup-cdc.cpp src/setup-cdc.hpp src/backend.hpp src/sysfs.hpp tests/setup-cdc.cpp tests/setup-cdc.sh LICENSE;do
 [[ -f $owner/$path && ! -L $owner/$path ]]
 install -m644 "$owner/$path" "$work/setup-cdc/$path"
done
(cd "$work/setup-cdc";find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
tar --sort=name --mtime=@1794787200 --owner=0 --group=0 --numeric-owner -cJf "$result/setup-cdc.tar.xz" -C "$work" setup-cdc
install -m644 "$owner/packaging/uke-control-setup-source-check.spec" "$result/"
[[ $(rpmspec --target aarch64 --define 'dist .fc46' -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$result/uke-control-setup-source-check.spec") == uke-control-setup-source-check-0.1-2.fc46 ]]
