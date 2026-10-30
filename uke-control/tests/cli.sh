#!/usr/bin/env bash
# Host-only CLI checks. Only static metadata, help and refusal paths are invoked.
set -Eeuo pipefail
cd "$(dirname "$0")/.."
export LD_LIBRARY_PATH="$PWD/build"
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
build/test-cli
app=$PWD/build/uke-control
"$app" </dev/null > "$stage/no-terminal"
grep -F 'Usage: uke-control' "$stage/no-terminal" >/dev/null
if "$app" --json </dev/null > "$stage/empty-json" 2> "$stage/empty-error"; then exit 1; else test "$?" = 64; fi
"$app" --version > "$stage/version"
version=$(awk -F '"' '/application_version=/ {print $2}' src/version.hpp)
grep -Fx "uke-control $version" "$stage/version"
"$app" --version --json | jq -e --arg version "$version" '.program=="uke-control" and .version==$version' >/dev/null
"$app" complete dis | grep -Fx display
"$app" complete --j --json | jq -e '.schema=="org.uke.control.completions.v1" and .candidates==["--json"]' >/dev/null
"$app" capabilities --json | jq -er '.commands[]' | while read -r command; do "$app" "$command" --help >/dev/null; done
for alias in channel branch brunch; do "$app" "$alias" --help > "$stage/help"; grep -q 'channel set' "$stage/help"; done
for alias in diagnostics diagnostic; do "$app" "$alias" --help > "$stage/help"; grep -q 'diagnostics all' "$stage/help"; done
"$app" charge plan charge80 > "$stage/human"
grep -Fx 'Applied: no' "$stage/human"
grep -Fx 'Effective: not reported' "$stage/human"
! grep -qE '^\s*\{|schema_version|"applied"' "$stage/human"
"$app" --json charge plan charge80 > "$stage/json"
jq -e '.applied==false and .effective==null and .requested=="charge80"' "$stage/json" >/dev/null
"$app" charge plan charge80 --json | cmp "$stage/json" -
"$app" charge modes > "$stage/modes"
grep -q 'Charging modes' "$stage/modes"
"$app" charge modes --json | jq -e '
    .qualification_checked==false and
    ([.modes[].id] as $ids |
     ($ids|length)==($ids|unique|length) and
     (["firmware","charge80","hold70-80","hold80-90","temporary-full","slow","automatic"] - $ids | length)==0) and
    any(.modes[]; .id=="temporary-full" and .apply_implemented==false)
' >/dev/null
"$app" charge plan temporary-full 30 --json | jq -e '
    .duration_minutes==30 and .armed==false and .applied==false and
    .hardware_write==false and .persisted==false and .effective==null
' >/dev/null
for args in 'unknown-command' 'usb typo' 'flashlight status --json --json' 'resize system --json' 'sensors -- --json' 'channel status -- --json' 'test -- --help' 'singleboot status -- --json' 'multiboot -- --help'; do
    read -r -a words <<< "$args"
    set +e
    "$app" "${words[@]}" > "$stage/out" 2> "$stage/error"
    result=$?
    set -e
    test "$result" = 64
    test ! -s "$stage/out"
    if [[ $args == 'flashlight status --json --json' || $args == 'resize system --json' ]]; then
        jq -e '.schema=="org.uke.control.error.v1" and .error.code=="USAGE_ERROR" and .exit_status==64' "$stage/error" >/dev/null
    else grep -q '^uke-control:' "$stage/error"; fi
done
# Help/version must exit before privileged device or package dispatch.
"$app" wake apply --help >/dev/null
"$app" resize system --version >/dev/null
"$app" channel set stable --help >/dev/null
# A usage refusal must occur before any package lock, RPM or repository access.
set +e
bash -c 'exec -a channel "$@"' bash build/uke-control-packages set stable --json > "$stage/out" 2> "$stage/error"
result=$?
set -e
test "$result" = 64 && test ! -s "$stage/out"
grep -q -- '--yes' "$stage/error"
printf '%s\n' 'Actual CLI: redirected human default, opt-in JSON, canonical/legacy help, stderr/exit contracts and no-action refusals PASS (host only)'
${CXX:-c++} -std=c++20 -Wall -Wextra -Werror -fPIC -shared -Isrc tests/cli-backend.cpp -o "$stage/backend.so"
export LD_PRELOAD="$stage/backend.so"
"$app" status > "$stage/status"
grep -q '^Swap:' "$stage/status"
"$app" status --json | jq -e '.schema=="org.uke.control.status.v1" and .swap.active==[]' >/dev/null
for args in 'usb role host' 'usb request data host'; do
    read -r -a words <<< "$args"
    "$app" "${words[@]}" --json | jq -e '.requested_role=="host" and .physical_acceptance==null' >/dev/null
done
for mode in human json; do
    flags=();test "$mode" != json || flags+=(--json)
    set +e
    "$app" charge mode slow "${flags[@]}" > "$stage/refused" 2> "$stage/error"
    result=$?
    set -e
    test "$result" = 69
    if test "$mode" = json; then jq -e '.applied==false and .effective==null' "$stage/refused" >/dev/null
    else grep -Fx 'Applied: no' "$stage/refused"; grep -Fx 'Effective: not reported' "$stage/refused"; fi
done
for args in 'charge review /synthetic/review.json' 'charge restore-current'; do
    read -r -a words <<< "$args"
    set +e
    "$app" "${words[@]}" > "$stage/refused" 2> "$stage/error"
    result=$?
    set -e
    test "$result" = 69
    test -s "$stage/refused" && test ! -s "$stage/error"
    ! grep -q '^{' "$stage/refused"
done
for query in status health cells; do
    "$app" battery "$query" --json | jq -e '.individual_cell_health==null and .view!=null' >/dev/null
    "$app" battery "$query" > "$stage/battery"
    grep -Fx 'Individual cell health: not reported' "$stage/battery"
    ! grep -q '^{' "$stage/battery"
done
set +e
"$app" battery protocol --json > "$stage/battery"
result=$?
set -e
test "$result" = 69
"$app" nfc status --json | jq -e '.available==false and .powered==null and .polling_started==false' >/dev/null
"$app" nfc status | grep -Fx 'Available: no'
unset LD_PRELOAD
printf '%s\n' 'Actual CLI with host-only backend fixture: coherent status, USB role aliases, explicit JSON and preserved EX_UNAVAILABLE result PASS'
