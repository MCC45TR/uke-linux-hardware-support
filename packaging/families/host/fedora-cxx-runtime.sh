#!/usr/bin/env bash
# Source metadata only; preserve programs and the real Fedora dependency.
set -Eeuo pipefail
input=${1:?source spec};output=${2:?rewritten spec}
case $(awk '/^Name:/ {print $2;exit}' "$input") in
    uke-core-meta|uke-installation-profile) expected=1;;
    uke-boot-integration) expected=4;;
    uke-battery-observer) expected=3;;
    *) echo 'Unreviewed GNU runtime consumer' >&2;exit 1;;
esac
[[ $(grep -c '^Requires:.*senemos-native-runtime(libstdc++)' "$input") == "$expected" ]]
awk '/^Requires:/ {gsub(/senemos-native-runtime[(]libstdc[+][+][)]/,"libstdc++%{?_isa} >= 16.2.1")} {print}' "$input" > "$output"
! grep -Fq 'senemos-native-runtime(libstdc++)' "$output"
[[ $(grep -Fc 'libstdc++%{?_isa} >= 16.2.1' "$output") == "$expected" ]]
