#!/usr/bin/bash
# COPR source assembly from the complete trusted signed21 package.
set -Eeuo pipefail
[[ $# == 1 && ${UKE_COPR_SOURCE_WORKER:-} == 1 && ! -e /etc/senemos/kernel-update.conf ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
result=$(realpath -m "$1")
[[ ! -e $result && ! -L $result ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$result" "$work/parent" "$work/pubkeys" "$work/gnupg"
curl -qfsSL --max-time 180 --max-filesize 25165824 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/Packages/u/uke-control-2.17.0-21.fc46.src.rpm -o "$work/parent.src.rpm"
printf '%s  %s\n' e51e2b310a8e51ae14d5746c367e984db0d54eeabb691970de432172c5686a4b "$work/parent.src.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.src.rpm" | cpio -t --quiet > "$work/paths"
awk '{name=$0;sub(/^\.\//,"",name);if(name!~/^[A-Za-z0-9_.+-]+$/||seen[name]++)bad=1;count++}END{if(bad||count!=73)exit 1}' "$work/paths"
rpm2cpio "$work/parent.src.rpm" | (cd "$work/parent";cpio -idm --quiet --no-absolute-filenames --no-preserve-owner)
fingerprint=$(GNUPGHOME="$work/gnupg" gpg --batch --with-colons --show-keys "$work/parent/copr-public-key.asc" 2>/dev/null | awk -F: '$1=="fpr"{print tolower($10);exit}')
[[ $fingerprint == 2204a661ea630a5303500085538797e72a3e4eca ]]
rpmkeys --define "_dbpath $work" --define '_keyring fs' --define "_keyringpath $work/pubkeys" --import "$work/parent/copr-public-key.asc"
rpmkeys --define "_dbpath $work" --define '_keyring fs' --define "_keyringpath $work/pubkeys" --checksig --verbose "$work/parent.src.rpm" > "$work/signatures"
[[ $(grep -Eic 'signature.*: OK$' "$work/signatures") == 2 ]]
! grep -Eiq 'NOTFOUND|NOKEY|BAD|NOT OK' "$work/signatures"
(cd "$work/parent";find . -maxdepth 1 -type f ! -name uke-control.spec -print0 | sort -z | xargs -0 sha256sum) > "$work/inputs.sha256"
bash "$owner/packaging/fold-boot-status-unstable-spec.sh" "$work/parent/uke-control.spec" "$work/new.spec"
cp -a "$work/parent/." "$result/"
install -m644 "$work/new.spec" "$result/uke-control.spec"
install -m644 "$owner/packaging/boot-status-unstable.patch" "$owner/tests/boot-status-unstable.sh" "$result/"
printf '%s  %s\n' e09e948f824c6c4133214cbcd3ca2fdefa05b8b20db449104d8ead857d6621d3 "$result/boot-status-unstable.patch" \
 0e906896cb8e16599b8a038fda55d0e28c9eaf59c370f1d1847a03fa6bbbed40 "$result/boot-status-unstable.sh" | sha256sum -c --strict --quiet -
(cd "$result";sha256sum -c --strict --quiet "$work/inputs.sha256")
[[ $(rpmspec --target aarch64 --define 'dist .fc46' -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$result/uke-control.spec") == uke-control-2.17.0-22.fc46 ]]
rpmspec --target aarch64 --define 'dist .fc46' -q --qf '%{NAME}\n' "$result/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui | cmp - "$work/names"
printf 'Control22: trusted signed21 source, all72 previous inputs and complete recipe inverse retained; native boot-status ELF tests run in RPM build.\n'
