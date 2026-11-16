#!/usr/bin/bash
set -Eeuo pipefail
[[ $# == 2 ]]
parent=$(realpath "$1");result=$(realpath -m "$2")
[[ ! -e $result && ! -L $result ]]
printf '%s  %s\n' 59d0ba80cf158ca94a0259453a1d8ffd1bd5141f22f5d05829e4f42c970779ce "$parent" | sha256sum -c --strict --quiet -
awk '
 /^Release: 12%\{\?dist\}$/{$0="Release: 13%{?dist}";release++}
 /^Source6: managed-firmware-full734.patch$/{print;print "Source7: managed-firmware-full7343.patch";source++;next}
 /^patch --batch --fuzz=0 -p1 < %\{SOURCE6\}$/{print;print "patch --batch --fuzz=0 -p1 < %{SOURCE7}";prepare++;next}
 /^%changelog$/{print;print "* Mon Nov 16 2026 Uke contributors - 0.2-13\n- Retain exact full7342 admission and qualify the successful full7343 source for idle provisioning.\n";history++;next}
 {print}END{if(release!=1||source!=1||prepare!=1||history!=1)exit 1}
' "$parent" > "$result"
awk '
 /^Release: 13%\{\?dist\}$/{$0="Release: 12%{?dist}"}
 /^Source7: managed-firmware-full7343.patch$/ || /^patch --batch --fuzz=0 -p1 < %\{SOURCE7\}$/{next}
 /^%changelog$/{print;drop=3;next}
 drop{drop--;next}
 {print}
' "$result" | cmp - "$parent"
printf 'Complete managed12 recipe inverse retained; only the exact qualified7343 entry and its native checks are added.\n'
