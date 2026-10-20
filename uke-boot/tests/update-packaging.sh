#!/usr/bin/env bash
set -Eeuo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
diff -u <(cat <<'HOOKS'
%post -n uke-kernel-update
%preun -n uke-kernel-update
%postun -n uke-kernel-update
%posttrans -n uke-installation-profile
%transfiletriggerin -n uke-kernel-update -- /usr/lib/senemos/kernel-updates
HOOKS
) <(grep -E '^%(pre|preun|pretrans|post|postun|posttrans|trigger[a-z]*|transfiletrigger[a-z]*)([[:space:]]|$)' packaging/uke-boot-integration.spec)
grep -Fxq 'automatic=0' src/profiles/kernel-update.conf
diff -u <(printf 'automatic=1\nvrr_min_hz=30\n') src/profiles/userdata-update.conf
grep -Fxq 'ConditionKernelCommandLine=root=PARTLABEL=userdata' src/units/uke-userdata-accept.service
grep -Fxq 'ExecStart=/usr/bin/uke-kernel-update auto' src/units/uke-userdata-accept.service
! grep -Eq '^After=.*graphical.target|^ProtectSystem=strict|^ExecStart=.*(bash|sh -c)' src/units/uke-userdata-accept.service
grep -Fq 'docs/USERDATA-AB.md' Makefile
grep -Fq 'Provides: uke-kernel-update(userdata-ab) = 1' packaging/uke-boot-integration.spec
printf '%s\n' 'Bounded transaction hooks, separate Linux/userdata policy, unit ordering and source export checked'
