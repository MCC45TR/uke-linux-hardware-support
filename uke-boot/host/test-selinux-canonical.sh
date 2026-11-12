#!/usr/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -Eeuo pipefail
[[ $# == 1 ]]
binary=$1
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cat > "$work/a" <<'CIL'
(class system (module_load))
(common extra (firmware_load))
(classcommon system extra)
(allow source target (system (module_load firmware_load)))
(booleanif enabled (true (allow source self (system (firmware_load)))))
(genfscon sysfs "/a(x);y" (system_u object_r sysfs_t ((s0) (s0))))
(genfscon sysfs "/b" (system_u object_r sysfs_t ((s0) (s0))))
(unknown_statement first second)
CIL
sed -e 's/(class system (module_load))/(class system (firmware_load module_load))/' \
    -e '/^(common extra /d' -e '/^(classcommon system /d' "$work/a" > "$work/b"
"$binary" full < "$work/a" | sort -u > "$work/a.full"
"$binary" full < "$work/b" | sort -u > "$work/b.full"
cmp "$work/a.full" "$work/b.full"
for change in \
    's/(true /(false /' \
    's/(allow source target /(allow target source /' \
    's/unknown_statement first second/unknown_statement second first/' \
    's/module_load firmware_load/module_load/'; do
    sed "$change" "$work/b" | "$binary" full | sort -u > "$work/changed"
    ! cmp -s "$work/b.full" "$work/changed"
done
awk 'NR==4{saved=$0;next}NR==5{print;print saved;next}{print}' "$work/b" \
    | "$binary" full | sort -u > "$work/reordered"
! cmp -s "$work/b.full" "$work/reordered"
printf '%s\n' '(broken' | "$binary" full > "$work/invalid" 2>/dev/null && exit 1
"$binary" old < "$work/a" | sort -u > "$work/old"
! grep -q firmware_load "$work/old"
printf '%s\n' 'Native canonicalizer common layout, permission, branch, direction, unknown-statement, label-order, malformed-input and legacy-permission checks passed.'
