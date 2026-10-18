#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Host fixture: no command inside a probe reaches a real device.
set -euo pipefail
source_dir=$(cd -- "$(dirname -- "$0")/.." && pwd)
fixture=$(mktemp -d)
trap 'rm -rf -- "$fixture"' EXIT
mkdir "$fixture/bin" "$fixture/output"
export AUDIO_PROBE_LOG="$fixture/probes.log"
cat > "$fixture/bin/id" <<'SH'
#!/bin/sh
printf '%s\n' "${AUDIO_FIXTURE_UID:-1000}"
SH
cat > "$fixture/bin/timeout" <<'SH'
#!/bin/sh
shift 3
printf '%s\n' "$*" >> "$AUDIO_PROBE_LOG"
case "$1" in
    rpm) printf 'synthetic missing package\n'; exit 3 ;;
    journalctl) printf 'synthetic permission denial\n' >&2; exit 1 ;;
    *) printf 'synthetic metadata\n' ;;
esac
SH
chmod +x "$fixture/bin/"*
ln -s timeout "$fixture/bin/rpm"
ln -s timeout "$fixture/bin/journalctl"
export PATH="$fixture/bin:$PATH"
cd "$fixture/output"
if AUDIO_FIXTURE_UID=0 sh "$source_dir/src/audio-inventory.sh" > /dev/null 2>&1; then
    echo 'Root collector was accepted' >&2; exit 1
fi
if sh "$source_dir/src/audio-inventory.sh" unexpected > /dev/null 2>&1; then
    echo 'Unexpected collector argument was accepted' >&2; exit 1
fi
test -z "$(ls -A)"
sh "$source_dir/src/audio-inventory.sh" > "$fixture/result.txt"
out=(uke-audio-inventory.*)
test "${#out[@]}" = 1
test "$(stat -c %a "${out[0]}")" = 700
test -z "$(find "${out[0]}" -type f -perm /077 -print)"
awk -F '\t' '$1=="audio-packages" && $3==3 {found=1} END {exit !found}' "${out[0]}/status.tsv"
awk -F '\t' '$1=="kernel-log" && $3==1 {found=1} END {exit !found}' "${out[0]}/status.tsv"
grep -q 'synthetic permission denial' "${out[0]}/probes/kernel-log.stderr"
grep -q 'rpmsg aprbus' "$AUDIO_PROBE_LOG"
grep -q 'alsa-utils alsa-ucm plasma-pa' "$AUDIO_PROBE_LOG"
test "$(wc -l < "${out[0]}/status.tsv")" = 12
(cd "${out[0]}" && sha256sum -c SHA256SUMS > /dev/null)
printf 'Audio inventory: refusal, private output, probe errors and checksums passed (host fixture).\n'
