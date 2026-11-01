#!/bin/bash
# Regular-file GIO fixture only; no daemon or hardware control is started.
set -Eeuo pipefail
export LC_ALL=C
scratch=$(mktemp -d)
trap 'find "$scratch" -depth -delete' EXIT
: > "$scratch/watched"
for mode in unset explicit; do
    environment=(env -u HOME -u XDG_RUNTIME_DIR -u XDG_CACHE_HOME -u XDG_CONFIG_HOME -u XDG_DATA_HOME)
    if [[ $mode == explicit ]]; then environment+=("HOME=$scratch"); fi
    status=0
    timeout --signal=TERM 2s strace -f -e trace=openat,inotify_add_watch -o "$scratch/$mode.trace" \
        "${environment[@]}" gio monitor "$scratch/watched" > "$scratch/$mode.out" 2> "$scratch/$mode.err" || status=$?
    [[ $status == 124 ]] || { cat "$scratch/$mode.err" >&2; exit 1; }
    grep -Eq 'inotify_add_watch\(.*\) = [0-9]+' "$scratch/$mode.trace"
done
grep -Fq '"/etc/passwd"' "$scratch/unset.trace"
if grep -Fq '"/etc/passwd"' "$scratch/explicit.trace"; then
    printf 'Explicit service HOME still requires the passwd database\n' >&2
    exit 1
fi
printf 'GIO monitor watches the file in both cases; explicit HOME avoids the NSS passwd read. Regular files only.\n'
