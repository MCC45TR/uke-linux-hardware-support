#!/bin/sh
# SPDX-License-Identifier: MIT
# Uke audio baseline collector. Does not open a playback/capture PCM.
# No vendor calibration, regmap, raw I2C or remoteproc control accesses.
# Run as the ordinary logged-in user. Do not publish raw output.
set -u
umask 077

if [ "$#" -ne 0 ]; then
    printf 'Usage: uke-control test audio-inventory (no playback or capture)\n' >&2
    exit 2
fi
uid=$(id -u) || exit 2
if [ "$uid" = 0 ]; then
    printf 'Run as the desktop user in a private working directory.\n' >&2
    exit 2
fi

for required in timeout mktemp date mkdir cat tr readlink find sha256sum; do
    if ! command -v "$required" >/dev/null 2>&1; then
        printf 'Required tool unavailable: %s\n' "$required" >&2
        exit 2
    fi
done

OUT=$(mktemp -d "$PWD/uke-audio-inventory.XXXXXXXX") || exit 2
mkdir "$OUT/probes" || exit 2
printf 'probe\tstarted_utc\texit_code\n' > "$OUT/status.tsv" || exit 2

run_probe() {
    label=$1
    shift
    started=$(date -u '+%Y-%m-%dT%H:%M:%SZ')
    if command -v "$1" >/dev/null 2>&1; then
        timeout --signal=TERM --kill-after=2s 20s "$@" \
            > "$OUT/probes/$label.stdout" \
            2> "$OUT/probes/$label.stderr"
        result=$?
    else
        : > "$OUT/probes/$label.stdout"
        printf 'Unavailable command: %s\n' "$1" \
            > "$OUT/probes/$label.stderr"
        result=127
    fi
    printf '%s\t%s\t%s\n' "$label" "$started" "$result" \
        >> "$OUT/status.tsv"
    # Individual failures remain in status.tsv; continue the inventory.
    return 0
}

run_probe kernel uname -a
run_probe asound-cards cat /proc/asound/cards
run_probe asound-pcm cat /proc/asound/pcm
run_probe asound-version cat /proc/asound/version
run_probe modules cat /proc/modules

run_probe device-tree sh -c '
    for key in model compatible; do
        path=/sys/firmware/devicetree/base/$key
        printf "== %s ==\n" "$key"
        if [ -r "$path" ]; then
            tr "\000" "\n" < "$path"
            printf "\n"
        else
            printf "UNAVAILABLE_OR_NOT_READABLE\n"
        fi
    done
'

run_probe remoteproc-metadata sh -c '
    found=no
    for dev in /sys/class/remoteproc/remoteproc*; do
        [ -d "$dev" ] || continue
        found=yes
        printf "== %s ==\n" "${dev##*/}"
        # Only remoteproc core identity/state fields, never state writes.
        for key in name state firmware; do
            printf "%s=" "$key"
            if [ -r "$dev/$key" ]; then
                cat "$dev/$key"
                printf "\n"
            else
                printf "UNAVAILABLE_OR_NOT_READABLE\n"
            fi
        done
    done
    [ "$found" = yes ] || printf "NO_VISIBLE_REMOTEPROC_ENTRIES\n"
'

run_probe bus-bindings sh -c '
    for bus in i2c soundwire slimbus rpmsg aprbus; do
        printf "== BUS %s ==\n" "$bus"
        found=no
        for dev in /sys/bus/$bus/devices/*; do
            [ -e "$dev" ] || continue
            found=yes
            printf "device=%s\n" "${dev##*/}"
            if [ ! -L "$dev/driver" ]; then
                printf "driver=UNBOUND\n"
            fi
            for link in driver of_node; do
                if [ -L "$dev/$link" ]; then
                    printf "%s=" "$link"
                    readlink -f "$dev/$link"
                fi
            done
        done
        [ "$found" = yes ] || printf "NO_VISIBLE_ENTRIES\n"
    done
'

run_probe sound-links sh -c '
    found=no
    for card in /sys/class/sound/card*; do
        [ -e "$card" ] || continue
        found=yes
        printf "card=%s\n" "${card##*/}"
        for link in device device/driver device/of_node; do
            if [ -L "$card/$link" ]; then
                printf "%s=" "$link"
                readlink -f "$card/$link"
            fi
        done
    done
    [ "$found" = yes ] || printf "NO_VISIBLE_SOUND_CARDS\n"
'

run_probe audio-packages rpm -q \
    pipewire pipewire-alsa pipewire-pulseaudio wireplumber \
    alsa-lib alsa-utils alsa-ucm plasma-pa

# Existing kernel log only. May be unavailable without extra permissions.
# Do not auto-sudo, lower dmesg restrictions or change journal access.
run_probe kernel-log journalctl -b -k --no-pager -n 800 -o short-monotonic

cat > "$OUT/README.txt" <<'NOTICE'
PRIVATE BASELINE INVENTORY
No playback or capture PCM was opened by this collector.
No vendor calibration or raw register interface was accessed.
No service was restarted and no firmware/control setting was written.
Missing files and denied permissions are not proof of absent hardware.
Review status.tsv and semantic content; collector completion is not a pass.
Hardware/audio acceptance: NOT_RUN.
Review and redact logs before any external sharing.
NOTICE

(
    cd "$OUT" || exit 2
    find . -type f ! -name SHA256SUMS -exec sha256sum {} + > SHA256SUMS
) || exit 2

printf 'Private inventory directory: %s\n' "$OUT"
printf 'Hardware/audio acceptance: NOT_RUN\n'
