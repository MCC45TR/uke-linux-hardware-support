#!/usr/bin/env bash
# Host-only source preparation refusals; no OEM bytes or network required.
set -Eeuo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
mkdir -p "$work/output/rpmbuild/input" "$work/notices"
reject() {
    if make -s -o local-rpms copr-srpm "PUBLICATION_REQUESTED=$1" \
        "outdir=$work/output" "NOTICE_FILES=$work/notices" > "$work/result.log" 2>&1; then
        echo 'Unrequested publication or unverified OEM notices accepted' >&2; exit 1
    fi
    [[ ! -e $work/output/rpmbuild/SRPMS ]]
}
reject no
printf 'Synthetic notice corruption\n' > "$work/notices/vendor-NOTICE.xml.gz"
reject yes
[[ $(rpmspec -q --qf '%{NAME}' packaging/uke-firmware.spec) == uke-firmware ]]
echo 'Firmware source contract: one canonical binary; absent publisher request and changed notices reject.'
