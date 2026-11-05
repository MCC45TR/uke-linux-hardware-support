#!/usr/bin/env bash
# COPR only. No real block device, stock firmware or update command is used.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
for compiler in g++ clang++; do
    for mode in ordinary sanitized; do
        flags=(-O2)
        if [[ $mode == sanitized ]]; then
            flags=(-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
        fi
        "$compiler" -std=c++20 -UNDEBUG -Wall -Wextra -Werror "${flags[@]}" \
            "$owner/tests/managed-firmware.cpp" -o "$work/after"
        "$work/after"
        "$compiler" -std=c++20 -UNDEBUG -Wall -Wextra -Werror "${flags[@]}" \
            -DUKE_MANAGED_OLD_BOUNDARY "$owner/tests/managed-firmware.cpp" -o "$work/before"
        set +e
        "$work/before" > "$work/before.log" 2>&1
        status=$?
        set -e
        [[ $status == 1 ]]
        grep -Fxq 'firmware preparation failure reached activation' "$work/before.log"
        printf 'Original 1.8.95 color-only boundary refused at required firmware assertion: %s/%s\n' "$compiler" "$mode"
        for mutant in missing-verification dry-preparation replacement-readonly; do
            mkdir -p "$work/$mutant/src" "$work/$mutant/tests"
            cp "$owner/tests/managed-firmware.cpp" "$work/$mutant/tests/"
            cp "$owner/src/o82-private-color.hpp" "$work/$mutant/src/"
            case $mutant in
                missing-verification)
                    sed 's/verify(release);/(void)verify;/' "$owner/src/managed-firmware.hpp" \
                        > "$work/$mutant/src/managed-firmware.hpp"
                    reason='firmware verification failure reached activation';;
                dry-preparation)
                    sed 's/if (!dry) prepare(release);/(void)dry; prepare(release);/' "$owner/src/managed-firmware.hpp" \
                        > "$work/$mutant/src/managed-firmware.hpp"
                    reason='firmware/color/activation order differs';;
                replacement-readonly)
                    sed 's/if (action == candidate) return false;/if (action == candidate) return action == "replace-trial";/' \
                        "$owner/src/managed-firmware.hpp" > "$work/$mutant/src/managed-firmware.hpp"
                    reason='mutating action was classified read-only';;
            esac
            "$compiler" -std=c++20 -UNDEBUG -Wall -Wextra -Werror "${flags[@]}" \
                "$work/$mutant/tests/managed-firmware.cpp" -o "$work/mutant"
            set +e
            "$work/mutant" > "$work/mutant.log" 2>&1
            status=$?
            set -e
            [[ $status == 1 ]]
            grep -Fxq "$reason" "$work/mutant.log"
            printf 'Required firmware mutant refused: %s/%s/%s\n' "$mutant" "$compiler" "$mode"
        done
        "$compiler" -std=c++20 -UNDEBUG -Wall -Wextra -Werror -Wno-unused-function "${flags[@]}" \
            "$owner/tests/managed-firmware-boot.cpp" -o "$work/boot-after" \
            $(pkg-config --cflags --libs fdisk zlib openssl libsystemd)
        "$work/boot-after"
        "$compiler" -std=c++20 -UNDEBUG -Wall -Wextra -Werror -Wno-unused-function "${flags[@]}" \
            -DUKE_MANAGED_UNGATED_BOOT "$owner/tests/managed-firmware-boot.cpp" -o "$work/boot-before" \
            $(pkg-config --cflags --libs fdisk zlib openssl libsystemd)
        set +e
        "$work/boot-before" > "$work/boot-before.log" 2>&1
        status=$?
        set -e
        [[ $status == 1 ]]
        grep -Fxq 'boot firmware preparation failure reached acceptance' "$work/boot-before.log"
        printf 'Original 1.8.96 boot acceptance refused at required firmware assertion: %s/%s\n' "$compiler" "$mode"
    done
done
