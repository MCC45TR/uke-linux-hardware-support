#!/usr/bin/env bash
# COPR only: complete actual class sources, real GLib and generated protobuf.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
source_dir=$(realpath "${1:?prepared libssc source directory}")
seam=$(realpath "${2:?C source inclusion seam}")
driver=$(realpath "${3:?C++ lifetime driver}")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
ulimit -c 0
mapfile -t dirs < <(find build/src -maxdepth 1 -type d -name 'libssc.so*.p')
[[ ${#dirs[@]} == 1 ]]
flags=(-UNDEBUG -O1 -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-cast-function-type)
if [[ ${SANITIZE:-0} == 1 ]]; then
 flags+=(-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
fi
read -ra cflags <<< "$(pkg-config --cflags gio-2.0 qmi-glib libprotobuf-c)"
read -ra libs <<< "$(pkg-config --libs gio-2.0 qmi-glib libprotobuf-c)"
names=(accelerometer gyroscope light proximity magnetometer compass)
for kind in "${!names[@]}"; do
 name=${names[kind]}
 pattern="*libssc-sensor-$name.c.o"
 mapfile -t replaced < <(find "${dirs[0]}" -type f -name "$pattern")
 [[ ${#replaced[@]} == 1 ]]
 mapfile -t objects < <(find "${dirs[0]}" -type f -name '*.o' ! -name "$pattern" | sort)
 (( ${#objects[@]} >= 8 ))
 cc -std=gnu11 "${flags[@]}" "${cflags[@]}" -I src -I build/src -I data -I build/data \
  "-DMEASUREMENT_SOURCE=\"$source_dir/libssc-sensor-$name.c\"" "-DMEASUREMENT_KIND=$kind" \
  -c "$seam" -o "$work/class.o"
 c++ -std=c++20 "${flags[@]}" "${cflags[@]}" "-DMEASUREMENT_KIND=$kind" \
  "$driver" "$work/class.o" "${objects[@]}" "${libs[@]}" -lm -o "$work/lifetime"
 if [[ ${EXPECT_UNOWNED:-0} == 1 ]]; then
  set +e
  G_DEBUG=fatal-criticals G_SLICE=always-malloc "$work/lifetime" > "$work/before.log" 2>&1
  status=$?
  set -e
  if [[ $status != 134 ]] || ! grep -Fq 'Queued measurement must retain its sensor' "$work/before.log"; then
   cat "$work/before.log";exit 1
  fi
  printf 'Original %s loses its queued sensor (expected status134)\n' "$name"
 else
  ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
   G_DEBUG=fatal-criticals G_SLICE=always-malloc "$work/lifetime"
 fi
done
if [[ ${EXPECT_UNOWNED:-0} == 1 ]]; then
 printf 'All six original measurement classes fail pending ownership (expected status134)\n'
 exit 134
fi
printf 'All six actual measurement class lifetime contracts passed; no transport opened.\n'
