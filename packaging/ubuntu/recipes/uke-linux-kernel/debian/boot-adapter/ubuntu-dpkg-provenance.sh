#!/bin/bash
# Host-only dpkg provenance for the shared dracut payload. No device actions,
# source compilation, service starts or security configuration changes.
uke_dpkg_record_initramfs() (
 set -euo pipefail
 export LC_ALL=C
 local work recipe host_root prefix database scratch
 work=$(realpath "${1:?work directory required}")
 recipe=$(realpath "${2:?shared src/boot directory required}")
 host_root=$(realpath "${3:-/}")
 prefix=${host_root%/}
 database=$prefix/var/lib/dpkg
 [[ -d $work/base && -f $database/status ]]
 scratch=$(mktemp -d)
 trap 'rm -rf "$scratch"' EXIT
 mkdir -p "$work/licenses"
 : > "$scratch/owners"
 : > "$scratch/content-index"
 printf 'path\tsha256\torigin_kind\torigin\n' > "$work/payload-provenance.tsv"
 # Dracut relocates package scripts into hook directories. Index exact source
 # bytes from its installed packages rather than claiming target-path ownership.
 for package in dracut-core dracut-install systemd udev; do
  dpkg-query --admindir "$database" -L "$package" > "$scratch/package-files" 2>/dev/null || continue
  while IFS= read -r source; do
   [[ $source == /* && -f $prefix$source && ! -L $prefix$source ]] || continue
   [[ $source != *$'\t'* && $source != *$'\n'* ]]
   printf '%s\t%s\t%s\n' "$(sha256sum "$prefix$source" | cut -d ' ' -f1)" "$package" "$source" >> "$scratch/content-index"
  done < "$scratch/package-files"
 done
 # Register only exact source assets and the two freshly built shared helpers.
 : > "$scratch/project-index"
 for source in "$recipe/dracut/"* "$work/dracut/modules.d/92uke-abl/uke-boot-status" "$work/dracut/modules.d/92uke-abl/uke-userdata-grow"; do
  [[ -f $source && ! -L $source ]] || continue
  printf '%s\t%s\n' "$(sha256sum "$source" | cut -d ' ' -f1)" "${source##*/}" >> "$scratch/project-index"
 done
 while IFS= read -r -d '' file; do
  local path digest owner origin kind resolved candidate
  path=/${file#"$work/base/"}
  [[ $path != *$'\t'* && $path != *$'\n'* ]]
  digest=$(sha256sum "$file" | cut -d ' ' -f1)
  owner=
  for source in "$path" "${path/#\/usr\/bin\//\/bin\/}" "${path/#\/usr\/sbin\//\/sbin\/}" "${path/#\/usr\/lib\//\/lib\/}"; do
   [[ -f $prefix$source ]] || continue
   resolved=$(realpath "$prefix$source")
   [[ $resolved == "$prefix/"* ]]
   # Resolve usrmerge/alternatives first. Ignore dpkg's diversion explanation
   # lines, which are not a package ownership claim for the selected bytes.
   for candidate in "${resolved#"$prefix"}" "$source"; do
    dpkg-query --admindir "$database" -S "$candidate" > "$scratch/path-owners" 2>/dev/null || continue
    owner=$(sed -n '/^[A-Za-z0-9][A-Za-z0-9+.:_-]*: \//s/: \/.*$//p' "$scratch/path-owners" | sort -u)
    [[ -n $owner ]] && break
   done
   [[ -n $owner ]] || continue
   [[ $owner != *$'\n'* && $owner =~ ^[A-Za-z0-9+.:_-]+$ ]] || { echo 'Ambiguous package ownership' >&2; exit 1; }
   # Ownership alone does not prove that the payload retained those bytes.
   cmp "$file" "$resolved"
   break
  done
  if [[ -n $owner ]]; then
   kind=dpkg-owner; origin=$owner
  else
   origin=$(awk -F '\t' -v hash="$digest" '$1==hash {print $2; exit}' "$scratch/project-index")
   if [[ -n $origin ]]; then
    kind=shared-project-source
   else
    owner=$(awk -F '\t' -v hash="$digest" '$1==hash {print $2; exit}' "$scratch/content-index")
    if [[ -n $owner ]]; then
     kind=dpkg-content-match; origin=$owner
    else
     # Generated non-executable metadata is recorded as generated, never as
     # owned by a package. Unknown executable payloads must be reviewed.
     if [[ -x $file || $(od -An -tx1 -N4 "$file" | tr -d ' \n') == 7f454c46 || $(od -An -tx1 -N2 "$file" | tr -d ' \n') == 2321 ]]; then
      echo "Unmapped executable initramfs payload: $path" >&2; exit 1
     fi
     case $path in /etc/*|/run/*|/usr/lib/dracut/*) :;; *) echo "Unmapped initramfs payload: $path" >&2; exit 1;; esac
     case $path in /etc/shadow|/etc/gshadow) echo 'Private account metadata refused' >&2; exit 1;; esac
     kind=generated-metadata; origin=dracut-output
    fi
   fi
  fi
  [[ -z $owner ]] || printf '%s\n' "$owner" >> "$scratch/owners"
  printf '%s\t%s\t%s\t%s\n' "$path" "$digest" "$kind" "$origin" >> "$work/payload-provenance.tsv"
 done < <(find "$work/base" -type f -print0 | sort -z)
 printf '%s\n' dracut-core dracut-install f2fs-tools >> "$scratch/owners"
 printf 'package\tversion\tarchitecture\tsource\tsource_version\n' > "$work/build-debs.tsv"
 sort -u "$scratch/owners" | while IFS= read -r package; do
  local record architecture copyright
  record=$(dpkg-query --admindir "$database" -W -f='${binary:Package}\t${Version}\t${Architecture}\t${source:Package}\t${source:Version}\n' "$package")
  architecture=$(cut -f3 <<< "$record")
  [[ $architecture == arm64 || $architecture == all ]] || { echo "Foreign boot userspace package: $package" >&2; exit 1; }
  printf '%s\n' "$record" >> "$work/build-debs.tsv"
  copyright=$(realpath "$prefix/usr/share/doc/${package%%:*}/copyright")
  [[ -f $copyright && $copyright == "$prefix/"* ]]
  install -Dm644 "$copyright" "$work/licenses/$package/copyright"
 done
 [[ -d $prefix/usr/share/common-licenses ]]
 cp -a "$prefix/usr/share/common-licenses" "$work/licenses/"
 for source in "$recipe/licenses/"*; do
  [[ -f $source && ! -L $source ]] || continue
  install -Dm644 "$source" "$work/licenses/uke-shared-boot/${source##*/}"
 done
)
if [[ ${BASH_SOURCE[0]} == "$0" ]]; then uke_dpkg_record_initramfs "$@"; fi
