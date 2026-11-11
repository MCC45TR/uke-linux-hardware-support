#!/usr/bin/bash
# COPR only; actual C++ functions with fake I/O, never a device operation.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
workspace=$(realpath "$1")
control=$workspace/uke-control
runtime=$workspace/uke-platform-runtime
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
for cc in g++ clang++;do
 for sanitize in 0 1;do
  flags=(-std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror)
  if [[ $sanitize == 1 ]];then flags+=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer);fi
  "$cc" "${flags[@]}" -I"$control/src" "$control/tests/charge-client.cpp" "$control/src/charge-client.cpp" $(pkg-config --cflags --libs libsystemd) -o "$work/client"
  "$work/client"
  "$cc" "${flags[@]}" -I"$control/src" "$control/tests/charge-intent.cpp" -o "$work/intent"
  "$work/intent"
  "$cc" "${flags[@]}" "$runtime/tests/charge-control.cpp" "$runtime/src/battery/control.cpp" -o "$work/control"
  "$work/control"
  "$cc" "${flags[@]}" "$runtime/tests/charge-io.cpp" "$runtime/src/battery/control.cpp" "$runtime/src/battery/control-profile.cpp" "$runtime/src/battery/record.cpp" $(pkg-config --cflags --libs libsystemd libcrypto) -o "$work/io"
  mkdir "$work/io-$sanitize-${cc//+/x}"
  "$work/io" "$work/io-$sanitize-${cc//+/x}"
  printf 'Actual Control charge bridge passed:%s/sanitize%s; fake I/O,no device request.\n' "$cc" "$sanitize"
 done
done
# Compile the actual complete helper and changed Control CLI/library once.
make -C "$runtime" CXX=g++ "$runtime/build/uke-charge-control"
make -C "$control" CXX=g++ -j4 build/uke-control
llvm-readelf -h "$runtime/build/uke-charge-control" "$control/build/uke-control" | grep -F AArch64
llvm-nm -C "$control/build/libuke-control.so.0" > "$work/control-symbols"
grep -F 'uke::charge_apply_preconfigured_cap()' "$work/control-symbols"
grep -F 'uke::charge_apply_preconfigured_cap(bool)' "$work/control-symbols"
printf 'Actual Control charge bridge linked: ARM64 helper/CLI and original zero-argument ABI retained; RPM migration and electrical effect unaccepted.\n'
