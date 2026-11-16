#!/usr/bin/bash
set -Eeuo pipefail
[[ $# == 2 ]]
parent=$(realpath "$1");result=$(realpath -m "$2")
[[ ! -e $result && ! -L $result ]]
printf '%s  %s\n' 105365a9fe9a45bd00c84582e16ffaffae46d5e67b42bde546e58c0e6044091a "$parent" | sha256sum -c --strict --quiet -
awk '
 /^Release: 13%\{\?dist\}$/{$0="Release: 14%{?dist}";release++}
 /^Source7: managed-firmware-full7343.patch$/{print;print "Source8: managed-firmware-full7344.patch";source++;next}
 /^patch --batch --fuzz=0 -p1 < %\{SOURCE7\}$/{print;print "patch --batch --fuzz=0 -p1 < %{SOURCE8}";prepare++;next}
 /^%changelog$/{print;print "* Mon Nov 16 2026 Uke contributors - 0.2-14\n- Retain every prior admission and qualify successful full7344 for idle provisioning.\n";history++;next}
 {print}END{if(release!=1||source!=1||prepare!=1||history!=1)exit 1}
' "$parent" > "$result"
awk '
 /^Release: 14%\{\?dist\}$/{$0="Release: 13%{?dist}"}
 /^Source8: managed-firmware-full7344.patch$/ || /^patch --batch --fuzz=0 -p1 < %\{SOURCE8\}$/{next}
 /^%changelog$/{print;drop=3;next}
 drop{drop--;next}
 {print}
' "$result" | cmp - "$parent"
echo 'Complete managed13 recipe inverse retained; only qualified7344 and its actual native tests are added.'
