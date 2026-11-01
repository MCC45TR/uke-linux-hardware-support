#!/bin/bash
# Host-only cache attestation. Root prepares the receipt before source helpers
# run as builder. Verification never executes a target payload.
set -euo pipefail
export LC_ALL=C
catalog() {
 local directory file path owner arch count=0
 printf 'path\tsha256\tpackage\n'
 for directory in /usr/lib/udev/hwdb.d /etc/udev/hwdb.d; do
  [[ -d $host$directory ]] || continue
  while IFS= read -r -d '' file; do
   [[ -f $file && ! -L $file ]]
   path=${file#"$host"}
   [[ $path != *$'\t'* && $path != *$'\n'* ]]
   owner=$(dpkg-query --admindir "$host/var/lib/dpkg" -S "$path" | sed -n '/^[A-Za-z0-9][A-Za-z0-9+.:_-]*: \//s/: \/.*$//p' | sort -u)
   [[ -n $owner && $owner != *$'\n'* && $owner =~ ^[A-Za-z0-9+.:_-]+$ ]]
   arch=$(dpkg-query --admindir "$host/var/lib/dpkg" -W -f='${Architecture}' "$owner")
   [[ $arch == arm64 || $arch == all ]]
   [[ $(stat -c %u "$file") == 0 && $((8#$(stat -c %a "$file") & 8#022)) == 0 ]]
   printf '%s\t%s\t%s\n' "$path" "$(sha256sum "$file" | cut -d ' ' -f1)" "$owner"
   ((count+=1))
  done < <(find "$host$directory" -maxdepth 1 \( -type f -o -type l \) -name '*.hwdb' -print0 | sort -z)
 done
 [[ $count -gt 0 ]]
}
command -v systemd-hwdb >/dev/null
if [[ ${1:-} == --prepare ]]; then
 [[ $# == 3 && $EUID == 0 ]]
 host=${2%/}; receipt=$3
 [[ ! -e $receipt && -f $host/var/lib/dpkg/status ]]
 mkdir -m755 "$receipt"
 catalog > "$receipt/inputs.tsv"
 # A temporary --root changes source paths embedded in hwdb.bin. Generate at
 # the original host paths so no disposable build paths enter the payload.
 if [[ -z $host ]]; then systemd-hwdb --usr update;
 else systemd-hwdb --root="$host" --usr update; fi
 sha256sum "$host/usr/lib/udev/hwdb.bin" | cut -d ' ' -f1 > "$receipt/cache.sha256"
 sha256sum "$(command -v systemd-hwdb)" | cut -d ' ' -f1 > "$receipt/generator.sha256"
 chmod 644 "$receipt/"*
 exit
fi
[[ $# == 3 && -f $1 ]]
payload=$1; host=${2%/}; output=$3
receipt=${UKE_HWDB_RECEIPT:-/run/uke-hwdb-provenance}
[[ -d $receipt && ! -L $receipt ]]
for path in "$receipt" "$receipt/inputs.tsv" "$receipt/cache.sha256" "$receipt/generator.sha256"; do
 [[ $(stat -c %u "$path") == 0 && $((8#$(stat -c %a "$path") & 8#022)) == 0 ]]
done
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
catalog > "$work/inputs.tsv"
cmp "$receipt/inputs.tsv" "$work/inputs.tsv"
[[ $(sha256sum "$(command -v systemd-hwdb)" | cut -d ' ' -f1) == $(cat "$receipt/generator.sha256") ]]
[[ $(sha256sum "$payload" | cut -d ' ' -f1) == $(cat "$receipt/cache.sha256") ]]
install -m644 "$receipt/inputs.tsv" "$output"
