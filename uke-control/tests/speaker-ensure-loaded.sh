#!/usr/bin/bash
# COPR only: real helper and linked ALSA shims; no audio device/private source.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
owner=$(realpath "${1:?Patched speaker runtime source}")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
ulimit -c 0
# The actual privileged helper makes itself nondumpable before private access.
# Address/undefined instrumentation remains enabled; ptrace-based leak checking
# is separate from these linked-shim CLI tests.
export ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1
for cxx in g++ clang++;do
 for sanitized in 0 1;do
  flags=(-O2)
  if [[ $sanitized == 1 ]];then flags=(-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer);fi
  "$cxx" -std=c++20 -Wall -Wextra -Werror "${flags[@]}" \
   "$owner/src/audio/uke-speaker-runtime.cpp" "$owner/tests/audio-speaker-control-mock.cpp" -lasound -lcrypto -o "$work/runtime"
  run() { env UKE_AUDIO_TEST_REQUIRE_LOCKED_READ=1 "$@" "$work/runtime" --ensure-loaded --acdb "$work/absent-acdb" --persist-directory "$work/absent-private"; }
  for state in 2 3 4;do
   run UKE_AUDIO_TEST_STATE=$state > "$work/result"
   grep -Fx 'Protected speaker parameters already staged; no data read or write.' "$work/result"
  done
  for state in 1 5 6;do
   if run UKE_AUDIO_TEST_STATE=$state > "$work/result" 2>&1;then exit 1;fi
   grep -Fx 'uke-speaker-runtime: speaker runtime operation rejected' "$work/result"
  done
  # Empty follows the original load path, so the absent generic source refuses.
  if run UKE_AUDIO_TEST_STATE=0 > "$work/result" 2>&1;then exit 1;fi
  ! grep -Fq 'already staged' "$work/result"
  for fault in NOT_ROOT LOCK_FAILURE READ_FAILURE ABSENT USER_COMMAND USER_STATE NONVOLATILE READABLE_COMMAND WRITABLE_STATE BAD_ENUM REPLACED_AFTER_LOCK;do
   if run UKE_AUDIO_TEST_STATE=4 "UKE_AUDIO_TEST_$fault=1" > "$work/result" 2>&1;then exit 1;fi
   ! grep -Fq 'already staged' "$work/result"
  done
  # The state used for the no-op decision must be sampled after the lock.
  for state in 1 5 6;do
   if run UKE_AUDIO_TEST_STATE=4 UKE_AUDIO_TEST_LOCKED_STATE=$state > "$work/result" 2>&1;then exit 1;fi
   grep -Fx 'uke-speaker-runtime: speaker runtime operation rejected' "$work/result"
  done
  for operation in --check --load --ensure-loaded --status --has-control --gain-readiness;do
   if UKE_AUDIO_TEST_STATE=4 "$work/runtime" --ensure-loaded "$operation" \
    --acdb "$work/absent-acdb" --persist-directory "$work/absent-private" > "$work/result" 2>&1;then exit 1;fi
  done
  # Explicit load remains explicit and still reaches its source checks.
  if UKE_AUDIO_TEST_STATE=4 "$work/runtime" --load --acdb "$work/absent-acdb" --persist-directory "$work/absent-private" > "$work/result" 2>&1;then exit 1;fi
  printf 'Actual speaker ensure-loaded:%s/sanitize%s; locked healthy no-op, fault/spoof/race refusal, no audio/calibration access.\n' "$cxx" "$sanitized"
 done
done
