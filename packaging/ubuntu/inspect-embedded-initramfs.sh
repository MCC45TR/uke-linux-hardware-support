#!/bin/bash
# Read-only Image inspection on a host. Never executes an extracted payload.
set -euo pipefail
export LC_ALL=C
[[ $# == 2 && -f $1 && ! -e $2 ]]
image=$(realpath "$1")
destination=$(realpath -m "$2")
[[ $(stat -c %s "$image") -le 100663296 ]]
for tool in bsdtar gzip grep dd head sha256sum; do command -v "$tool" >/dev/null; done
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
found=0
while IFS= read -r offset; do
 [[ $offset =~ ^[0-9]+$ ]]
 set +e
 dd if="$image" iflag=skip_bytes skip="$offset" status=none | gzip -dc 2> "$work/gzip.log" | head -c268435457 > "$work/candidate.cpio"
 status=("${PIPESTATUS[@]}")
 set -e
 [[ ${status[1]} == 0 || ${status[1]} == 2 ]] || continue
 [[ $(stat -c %s "$work/candidate.cpio") -le 268435456 ]]
 [[ $(head -c6 "$work/candidate.cpio") == 070701 ]] || continue
 bsdtar -tf "$work/candidate.cpio" > "$work/paths"
 sed -i 's|^\./||' "$work/paths"
 grep -Fxq usr/libexec/uke-root-guard "$work/paths" || continue
 # No duplicate/escaping paths. The pinned producer explicitly encodes only
 # the standard console node; validate its metadata, never create that node.
 [[ -z $(sort "$work/paths" | uniq -d) ]]
 while IFS= read -r path; do
  [[ $path =~ ^[A-Za-z0-9._@:+/-]+$ && $path != /* && $path != *..* ]]
 done < "$work/paths"
 console_count=$(bsdtar -tvf "$work/candidate.cpio" | awk '
  /^[-dl]/ {next}
  $1=="crw-------" && $2==1 && $3==0 && $4==0 && $5=="5,1" && $NF=="dev/console" {console++;next}
  {bad=1}
  END {if(bad || console>1)exit 1;print console+0}
 ')
 ((found+=1))
 [[ $found == 1 ]]
 cp "$work/candidate.cpio" "$work/accepted.cpio"
 accepted_console_count=$console_count
done < <(grep -aob "$(printf '\037\213\010')" "$image" | cut -d: -f1)
[[ $found == 1 ]] || { echo 'Exactly one embedded Uke initramfs is required' >&2; exit 1; }
mkdir -m700 "$destination"
bsdtar -xf "$work/accepted.cpio" -C "$destination" --no-same-owner --no-same-permissions --exclude dev/console --exclude ./dev/console
printf '%s  %s\n' 36164c87aaf1734248f57c6fbdb8dec7c5d2768526b2551ad3dfc1477e7f83d4 "$destination/usr/libexec/uke-root-guard" | sha256sum -c -
[[ -x $destination/usr/lib/systemd/systemd && -x $destination/usr/libexec/uke-offline-switch-root ]]
[[ $accepted_console_count == 1 && ! -e $destination/dev/console ]]
while IFS= read -r -d '' path; do
 if [[ $(od -An -tx1 -N4 "$path" | tr -d ' \n') == 7f454c46 ]]; then
  readelf -h "$path" | grep -Eq 'Machine:.*AArch64'
 elif [[ $(head -c2 "$path") == '#!' ]]; then
  if head -n1 "$path" | grep -Ei 'python|pypy'; then exit 1; fi
 fi
done < <(find "$destination" -type f -print0)
[[ ! -e $destination/etc/shadow && ! -L $destination/etc/shadow && ! -e $destination/etc/gshadow && ! -L $destination/etc/gshadow ]]
[[ ! -e $destination/usr/lib/firmware && ! -L $destination/usr/lib/firmware && ! -s $destination/etc/machine-id ]]
[[ -z $(find "$destination" -type f \( -name '*.py' -o -name '*.pyc' -o -iname '*python*' -o -iname '*pypy*' \) -print -quit) ]]
printf 'Extracted embedded Uke initramfs and verified the pinned root guard; no target executable ran.\n'
printf 'Validated console node: char 5:1, uid/gid 0, mode 0600; node was not created.\n'
