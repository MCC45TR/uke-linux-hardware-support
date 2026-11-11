#!/usr/bin/bash
# COPR only. Actual helper wait and existing fake-I/O tests; no device main.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
runtime=$(realpath "${1:?Prepared actual helper source}")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
common=("$runtime/src/battery/control.cpp" "$runtime/src/battery/control-profile.cpp" "$runtime/src/battery/record.cpp")
read -r -a libs <<< "$(pkg-config --cflags --libs libsystemd libcrypto)"
wrap=(-Wl,--wrap=ppoll -Wl,--wrap=pthread_sigmask)
for cc in g++ clang++;do
 for sanitize in 0 1;do
  flags=(-std=c++20 -O2 -g -UNDEBUG -Wall -Wextra -Werror -pthread)
  if [[ $sanitize == 1 ]];then flags+=(-O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie);fi
  "$cc" "${flags[@]}" "$runtime/tests/charge-wait.cpp" "${common[@]}" "${libs[@]}" "${wrap[@]}" -o "$work/wait"
  "$work/wait"
  "$cc" "${flags[@]}" "$runtime/tests/charge-control.cpp" "$runtime/src/battery/control.cpp" -o "$work/control"
  "$work/control"
  "$cc" "${flags[@]}" "$runtime/tests/charge-io.cpp" "${common[@]}" "${libs[@]}" -o "$work/io"
  mkdir "$work/io-$sanitize-${cc//+/x}"
  "$work/io" "$work/io-$sanitize-${cc//+/x}"
  printf 'Actual Control wait and retained charge/IO passed:%s/sanitize%s; no device main or firmware operation.\n' "$cc" "$sanitize"
 done
done
ulimit -c 0
for mutation in early-deadline ignore-interrupt;do
 bad=$work/$mutation
 cp -a "$runtime" "$bad"
 node - "$bad/src/battery/control-main.cpp" "$mutation" <<'NODE'
const fs=require('fs'),[p,mode]=process.argv.slice(2);let s=fs.readFileSync(p,'utf8'),old,next;
if(mode==='early-deadline'){old='if (remaining <= std::chrono::steady_clock::duration::zero()) break;';next='if (remaining > std::chrono::steady_clock::duration::zero()) break;';}
else if(mode==='ignore-interrupt'){old='if (result < 0 && errno == EINTR) continue;';next='if (result < 0 && errno == EINTR) break;';}else throw Error('Unknown mutant');
if(s.split(old).length!==2)throw Error('Actual wait mutation anchor');s=s.replace(old,()=>next);fs.writeFileSync(p,s);
NODE
 g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -pthread "$bad/tests/charge-wait.cpp" "$bad/src/battery/control.cpp" "$bad/src/battery/control-profile.cpp" "$bad/src/battery/record.cpp" "${libs[@]}" "${wrap[@]}" -o "$work/mutant"
 status=0
 "$work/mutant" > "$work/$mutation.log" 2>&1 || status=$?
 [[ $status == 134 ]]
 printf 'Actual wait regression refused:%s/status%s.\n' "$mutation" "$status"
done
make -C "$runtime" CXX=g++ "$runtime/build/uke-charge-control"
llvm-readelf -h "$runtime/build/uke-charge-control" | grep -F AArch64
printf 'Actual Control charge wait linked: full ARM64 helper,unchanged hardware bodies and no extra runtime dependencies; target wakeup/electrical acceptance pending.\n'
