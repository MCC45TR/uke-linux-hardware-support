#!/usr/bin/bash
# COPR source worker: exact public factory data, never private unit data.
set -Eeuo pipefail
[[ $# == 1 && ${UKE_COPR_SOURCE_WORKER:-} == 1 && $(uname -m) == aarch64 && ! -e /etc/senemos/kernel-update.conf ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
result=$(realpath -m "$1");manifest=$owner/manifests/display-inputs-20261117.json
[[ ! -e $result && ! -L $result ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$result" "$work/parent"
curl -qfsSL --max-time 240 --max-filesize 33554432 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11332072/uke-firmware-3.0.304.0-17.src.rpm -o "$work/parent.src.rpm"
printf '%s  %s\n' a6f1caadf8965469743cbeb6dc85c60c17d155e274042367a2975acf1a65a8c0 "$work/parent.src.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.src.rpm" | cpio -t --quiet > "$work/paths"
awk '{name=$0;sub(/^\.\//,"",name);if(name!~/^[A-Za-z0-9_.+-]+$/||seen[name]++)bad=1;count++}END{if(bad||count!=9)exit 1}' "$work/paths"
rpm2cpio "$work/parent.src.rpm" | (cd "$work/parent";cpio -idm --quiet --no-absolute-filenames --no-preserve-owner)
(cd "$work/parent";find . -maxdepth 1 -type f ! -name uke-firmware.spec -print0 | sort -z | xargs -0 sha256sum) > "$work/retained.sha256"
bash "$owner/packaging/fold-display-inputs18.sh" "$work/parent/uke-firmware.spec" "$work/new.spec"
cp -a "$work/parent/." "$result/"
install -m644 "$work/new.spec" "$result/uke-firmware.spec"
install -m644 "$manifest" "$result/"
while IFS=$'\t' read -r name repo commit path bytes hash;do
 [[ $name =~ ^(o82-(36|42)\.json|vpu30_2v\.mbn)$ && $commit =~ ^[0-9a-f]{40}$ && $hash =~ ^[0-9a-f]{64}$ && $bytes =~ ^[0-9]+$ ]]
 curl -qfsSL --max-time 120 --max-filesize "$bytes" "https://raw.githubusercontent.com/$repo/$commit/$path" -o "$result/$name"
 [[ $(stat -c %s "$result/$name") == "$bytes" ]]
 printf '%s  %s\n' "$hash" "$result/$name" | sha256sum -c --strict --quiet -
done < <(jq -er '.inputs[]|[.name,.repository,.commit,.path,.bytes,.sha256]|@tsv' "$manifest")
(cd "$result";sha256sum -c --strict --quiet "$work/retained.sha256")
printf 'All eight firmware17 inputs retained; three already admitted public factory inputs added. No daemon, private calibration or firmware execution.\n'
