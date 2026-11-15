#!/usr/bin/bash
# COPR-native: original legacy fixtures plus modern identity through the real ELF.
set -Eeuo pipefail
[[ $# == 2 ]]
root=$(realpath "$1");production=$(realpath "$2")
[[ -x $production ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
(cd "$root";bash tests/boot-status.sh)
for compiler in g++ clang++;do
 "$compiler" -std=c++20 -O1 -g -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-sanitize-recover=all \
  "$root/src/boot-status.cpp" -o "$work/$compiler"
done
fixture=$work/root
mkdir -p "$fixture/sys/firmware/devicetree/base" "$fixture/proc/sys/kernel"
printf 'xiaomi,uke\0qcom,sm7675\0' > "$fixture/sys/firmware/devicetree/base/compatible"
printf 'root=PARTLABEL=userdata rw\n' > "$fixture/proc/cmdline"
for binary in "$production" "$work/g++" "$work/clang++";do
 for release in 7.3.1-1-unstable 7.3.3-1-unstable 7.3.3-28-unstable 7.3.4-2-unstable 7.4.0-1-unstable 8.0.0-1-unstable;do
  printf '%s\n' "$release" > "$fixture/proc/sys/kernel/osrelease"
  "$binary" --identity --root "$fixture" | jq -e \
   '.uke_compatible and .selected_kernel_release and (.hardware_acceptance_granted|not)' >/dev/null
 done
 for release in '' 7.3.0-1-unstable 7.3.3 7.3.3-0-unstable 7.3.3-01-unstable 7.3.03-1-unstable \
  7.3.3-4294967296-unstable 7.3.3--unstable 7.3.3-1-foreign 7.3.3-1.2-unstable \
  7.3.3-1-extra-unstable 7.3.3-1-unstable-extra 7.3.4-2-senemos 7.3.3-1-unstable/foreign;do
  printf '%s\n' "$release" > "$fixture/proc/sys/kernel/osrelease"
  if "$binary" --identity --root "$fixture" > "$work/refusal.json";then exit 1;else [[ $? == 2 ]];fi
  jq -e '(.selected_kernel_release|not) and (.hardware_acceptance_granted|not)' "$work/refusal.json" >/dev/null
 done
 printf '7.3.4-2-unstable\n' > "$fixture/proc/sys/kernel/osrelease"
 for compatible in 'xiaomi,nabu\0qcom,sm7675\0' 'xiaomi,uke\0qcom,sm8635\0' 'xiaomi,uke\0qcom,sm7675';do
  printf '%b' "$compatible" > "$fixture/sys/firmware/devicetree/base/compatible"
  if "$binary" --identity --root "$fixture" > "$work/refusal.json";then exit 1;else [[ $? == 2 ]];fi
  jq -e '(.uke_compatible|not) and (.hardware_acceptance_granted|not)' "$work/refusal.json" >/dev/null
 done
 printf 'xiaomi,uke\0qcom,sm7675\0' > "$fixture/sys/firmware/devicetree/base/compatible"
done
printf 'Production boot-status ELF and GCC/Clang sanitizers accept canonical unstable names and preserve legacy, malformed version, exact board and read-only identity refusals; no USB or hardware acceptance.\n'
