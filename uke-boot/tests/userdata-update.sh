#!/usr/bin/env bash
# Host-only state/transport fixtures and foreign-host no-write check.
set -Eeuo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p build/tests
compiler=${CXX:-c++}
flags=(-std=c++20 -O1 -g -Wall -Wextra -Werror -Wno-unused-function)
if [[ ${SANITIZE:-0} == 1 ]]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
for fixture in userdata-update ufs-boot-lun; do
    "$compiler" "${flags[@]}" "tests/$fixture.cpp" -o "build/tests/$fixture" $(pkg-config --cflags --libs fdisk zlib openssl libsystemd)
    "build/tests/$fixture"
done
"$compiler" -std=c++20 -O2 -Wall -Wextra -Werror src/kernel-update.cpp \
    -o build/tests/uke-kernel-update $(pkg-config --cflags --libs fdisk zlib openssl libsystemd)
if [[ $(id -u) == 0 && ! -e /sys/firmware/devicetree/base/compatible ]]; then
    build/tests/uke-kernel-update auto
    build/tests/uke-kernel-update transaction /usr/lib/senemos/kernel-updates/fixture
    for action in check-restore-current restore-current boot fallback check-auto; do
        args=("$action"); [[ $action == boot || $action == fallback || $action == check-auto ]] || args+=(/usr/lib/senemos/kernel-updates/fixture)
        if build/tests/uke-kernel-update "${args[@]}" >build/tests/restore-refusal.log 2>&1; then exit 1; fi
        if grep -q 'Usage:' build/tests/restore-refusal.log; then exit 1; fi
        grep -Fxq 'uke-kernel-update: Exact Uke/SM7675 identity required' build/tests/restore-refusal.log
    done
    printf 'Root foreign-host automatic no-op and strict action identity refusals passed\n'
elif [[ $(id -u) != 0 ]]; then
    for action in boot auto check-auto; do
        if build/tests/uke-kernel-update "$action" >build/tests/privilege-refusal.log 2>&1; then exit 1; fi
        grep -Fxq 'uke-kernel-update: Root privileges are required' build/tests/privilege-refusal.log
    done
    printf 'Unprivileged CLI recognition and root-required refusals passed; root foreign-host identity cases were not executed\n'
else
    printf 'Root foreign-host identity cases skipped: a Device Tree is visible\n'
fi
