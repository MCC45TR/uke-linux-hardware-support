#!/usr/bin/bash
# Data-only original-systemd test, runnable on PC and native COPR workers.
set -Eeuo pipefail
[[ $# == 1 ]]
config=$(realpath "$1")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
factory=$work/usr/lib/firmware/qcom/uke/audio/Forte_acdb_cal.acdb
cache=$work/var/cache/uke-speaker-runtime/Forte_acdb_cal.acdb
mkdir -p "$(dirname "$factory")" "$work/usr/lib/tmpfiles.d"
printf 'public synthetic data fixture\n' > "$factory"
chmod 444 "$factory"
install -m644 "$config" "$work/usr/lib/tmpfiles.d/uke-speaker-data.conf"
expected=$(sha256sum "$factory" | cut -d' ' -f1)
create() {
 [[ -f $factory && ! -L $factory ]] || return 1
 printf '%s  %s\n' "$expected" "$factory" | sha256sum -c --strict --quiet - || return 1
 systemd-tmpfiles --root="$work" --create "$work/usr/lib/tmpfiles.d/uke-speaker-data.conf"
}
create
[[ -f $cache && ! -L $cache && $(stat -c %a "$cache") == 444 ]]
cmp "$factory" "$cache"
before=$(stat -c '%i:%s:%Y' "$cache")
create
[[ $(stat -c '%i:%s:%Y' "$cache") == "$before" ]]
# Existing content, including an invalid entry, is not silently overwritten.
chmod 644 "$cache";printf 'existing fixture\n' > "$cache";chmod 444 "$cache"
create
[[ $(cat "$cache") == 'existing fixture' ]]
unlink "$cache"
ln -s /missing-synthetic-target "$cache"
create
[[ -L $cache && $(readlink "$cache") == /missing-synthetic-target ]]
unlink "$cache"
create
cmp "$factory" "$cache"
# Missing immutable source must fail rather than produce a partial cache.
unlink "$cache";unlink "$factory"
if create > "$work/missing.log" 2>&1;then echo 'Missing factory source admitted' >&2;exit 1;fi
[[ ! -e $cache && ! -L $cache ]]
printf 'Generic audio cache: original systemd fresh copy, repeat identity, existing-content and symlink preservation, cache recreation and explicit missing-source hash-guard refusal passed.\n'
