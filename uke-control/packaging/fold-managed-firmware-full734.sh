#!/usr/bin/bash
# Retain complete11 recipe; admit one exact current tuple after full success.
set -Eeuo pipefail
[[ $# == 2 ]]
parent=$(realpath "$1");result=$(realpath -m "$2")
[[ ! -e $result && ! -L $result ]]
printf '%s  %s\n' dc33a070f47edfb4b541e0af28f8e92e3d1ff1a17a88f32d1e23a679eef31150 "$parent" | sha256sum -c --strict --quiet -
awk '
 /^Release: 11%\{\?dist\}$/{$0="Release: 12%{?dist}";release++}
 /^Source5: factory-mcu-test.hpp$/{print;print "Source6: managed-firmware-full734.patch";source++;next}
 /^install -m644 %\{SOURCE5\} uke-cdsp-pogo-firmware\/tests\/factory-mcu-test.hpp$/{print;print "patch --batch --fuzz=0 -p1 < %{SOURCE6}";prepare++;next}
 /^%changelog$/{print;print "* Mon Nov 16 2026 Uke contributors - 0.2-12\n- Admit the exact successful full734-2 source for protected fresh-root provisioning.\n";history++;next}
 {print}END{if(release!=1||source!=1||prepare!=1||history!=1)exit 1}
' "$parent" > "$result"
awk '
 /^Release: 12%\{\?dist\}$/{$0="Release: 11%{?dist}"}
 /^Source6: managed-firmware-full734.patch$/ || /^patch --batch --fuzz=0 -p1 < %\{SOURCE6\}$/{next}
 /^%changelog$/{print;drop=3;next}
 drop{drop--;next}
 {print}
' "$result" | cmp - "$parent"
printf 'Complete managed11 recipe inverse passed; factory source, all old native tests and firmware ownership guards retained.\n'
