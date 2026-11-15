#!/usr/bin/bash
# Data-only recipe transformation with a complete inverse.
set -Eeuo pipefail
[[ $# == 2 ]]
parent=$(realpath "$1");result=$(realpath -m "$2")
[[ ! -e $result && ! -L $result ]]
printf '%s  %s\n' 0a9a2334a986454410915dde9af652c1a8984524968500a57ff40639bcb08d81 "$parent" | sha256sum -c --strict --quiet -
awk '
 /^Release: 9%\{\?dist\}$/{$0="Release: 10%{?dist}";release++}
 /^Source2: managed-firmware-version.patch$/{print;print "Source3: managed-firmware-idle28.patch";source++;next}
 /^patch --batch --fuzz=0 -p1 < %\{SOURCE2\}$/{print;print "patch --batch --fuzz=0 -p1 < %{SOURCE3}";prepare++;next}
 /^%changelog$/{print;print "* Mon Nov 16 2026 Uke contributors - 0.2-10\n- Admit exact full28 source with unchanged offline consumer and private-stock guards.\n";history++;next}
 {print}END{if(release!=1||source!=1||prepare!=1||history!=1)exit 1}
' "$parent" > "$result"
awk '
 /^Release: 10%\{\?dist\}$/{$0="Release: 9%{?dist}"}
 /^Source3: managed-firmware-idle28.patch$/{next}
 /^patch --batch --fuzz=0 -p1 < %\{SOURCE3\}$/{next}
 /^%changelog$/{print;drop=3;next}
 drop{drop--;next}
 {print}
' "$result" | cmp - "$parent"
printf 'Complete firmware9 recipe inverse passed.\n'
