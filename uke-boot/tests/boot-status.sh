#!/usr/bin/env bash
set -Eeuo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/tests
c++ -std=c++17 -Wall -Wextra -Werror -O2 src/boot-status.cpp -o build/tests/uke-boot-status
fixture=$(mktemp -d build/tests/root.XXXXXX)
baseline=$fixture-baseline
trap 'rm -rf "$fixture" "$baseline"' EXIT
mkdir -p "$fixture/sys/firmware/efi" "$fixture/sys/firmware/devicetree/base" "$fixture/proc/sys/kernel"
printf 'xiaomi,uke\0qcom,sm7675\0' > "$fixture/sys/firmware/devicetree/base/compatible"
printf '7.2.9-senemos-uke\n' > "$fixture/proc/sys/kernel/osrelease"
printf 'root=PARTLABEL=uke_linux rw rootwait\n' > "$fixture/proc/cmdline"
build/tests/uke-boot-status --root "$fixture" > build/tests/accepted.json
jq -e '.efi_present and .uke_compatible and .selected_kernel_release and .uke_root_command_line and (.hardware_acceptance_granted|not)' build/tests/accepted.json >/dev/null
cp -a "$fixture" "$baseline"
printf 'xiaomi,nabu\0qcom,sm8150\0' > "$fixture/sys/firmware/devicetree/base/compatible"
if build/tests/uke-boot-status --root "$fixture" > build/tests/foreign.json; then exit 1; else test "$?" -eq 2; fi
printf 'xiaomi,uke\0qcom,sm7675\0' > "$fixture/sys/firmware/devicetree/base/compatible"
printf 'root=PARTLABEL=uke_linux root=PARTLABEL=foreign rw\n' > "$fixture/proc/cmdline"
if build/tests/uke-boot-status --root "$fixture" > build/tests/foreign-root.json; then exit 1; else test "$?" -eq 2; fi
printf 'root=PARTLABEL=uke_linux rw rootwait\n' > "$fixture/proc/cmdline"
rmdir "$fixture/sys/firmware/efi"
if build/tests/uke-boot-status --root "$fixture" > build/tests/missing-efi.json; then exit 1; else test "$?" -eq 2; fi
mkdir "$fixture/sys/firmware/efi"
diff -r "$fixture" "$baseline" >/dev/null
build/tests/uke-boot-status --help >/dev/null
printf '%s\n' 'Boot prerequisite fixtures passed; foreign devices/root overrides rejected; inspection made no writes'
