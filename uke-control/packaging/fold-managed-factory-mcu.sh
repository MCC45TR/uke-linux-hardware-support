#!/usr/bin/bash
# Preserve the full10 managed recipe and the exact idle-current source guards.
set -Eeuo pipefail
[[ $# == 2 ]]
parent=$(realpath "$1");result=$(realpath -m "$2")
[[ ! -e $result && ! -L $result ]]
printf '%s  %s\n' e4448027862eb8889b277b58f5ad3e62e8ca0fcf0705c2697187583b203ef5bf "$parent" | sha256sum -c --strict --quiet -
awk '
 /^Release: 10%\{\?dist\}$/{$0="Release: 11%{?dist}";release++}
 /^Source3: managed-firmware-idle28.patch$/{print;print "Source4: managed-firmware-factory-mcu.patch\nSource5: factory-mcu-test.hpp\nBuildRequires: uke-firmware(factory-pogo-mcu-v1) >= 1\nRequires: uke-firmware(factory-pogo-mcu-v1) >= 1";source++;next}
 /^patch --batch --fuzz=0 -p1 < %\{SOURCE3\}$/{print;print "patch --batch --fuzz=0 -p1 < %{SOURCE4}\ninstall -m644 %{SOURCE5} uke-cdsp-pogo-firmware/tests/factory-mcu-test.hpp";prepare++;next}
 $0=="image from local read-only stock partitions. Publishes only non-running,"{$0="image from read-only packaged or local stock sources. Publishes only non-running,";description++}
 /^%changelog$/{print;print "* Mon Nov 16 2026 Uke contributors - 0.2-11\n- Use a protected exact-hash public MCU source before the unchanged stock fallback.\n";history++;next}
 {print}END{if(release!=1||source!=1||prepare!=1||description!=1||history!=1)exit 1}
' "$parent" > "$result"
awk '
 /^Release: 11%\{\?dist\}$/{$0="Release: 10%{?dist}"}
 /^Source4: managed-firmware-factory-mcu.patch$/ || /^Source5: factory-mcu-test.hpp$/ || /^(BuildRequires|Requires): uke-firmware\(factory-pogo-mcu-v1\) >= 1$/{next}
 /^patch --batch --fuzz=0 -p1 < %\{SOURCE4\}$/ || /^install -m644 %\{SOURCE5\} uke-cdsp-pogo-firmware\/tests\/factory-mcu-test.hpp$/{next}
 $0=="image from read-only packaged or local stock sources. Publishes only non-running,"{$0="image from local read-only stock partitions. Publishes only non-running,"}
 /^%changelog$/{print;drop=3;next}
 drop{drop--;next}
 {print}
' "$result" | cmp - "$parent"
printf 'Complete managed10 recipe inverse passed; idle source/active consumer/no-replace gates retained.\n'
