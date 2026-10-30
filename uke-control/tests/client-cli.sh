#!/usr/bin/env bash
set -Eeuo pipefail
app=build/uke-control
"$app" actions --json | jq -e '.schema == "org.uke.control.actions.v1" and .metadata_only and .physical_acceptance == null' >/dev/null
"$app" actions describe charge.mode --json | jq -e '.id == "charge.mode" and (.parameters[0].choices | length > 0)' >/dev/null
"$app" actions plan charge.mode mode=firmware --json | jq -e '.executed == false and .confirmation_required and .argv == ["/usr/bin/uke-control","charge","mode","firmware"]' >/dev/null
for arg in 'mode=INVALID' 'nonsense=value'; do
    status=0
    "$app" actions plan charge.mode "$arg" --json >/dev/null || status=$?
    test "$status" = 64
done
status=0
"$app" actions plan charge.mode mode=firmware mode=slow --json >/dev/null || status=$?
test "$status" = 64
status=0
"$app" tui --json >/dev/null || status=$?
test "$status" = 64
"$app" tui --help | grep -F -- '--read-only' >/dev/null
"$app" complete tui | grep -Fx tui >/dev/null
printf '%s\n' 'CLI metadata planning passed without executing a device action'
