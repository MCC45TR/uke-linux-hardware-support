#!/usr/bin/bash
# Native COPR fixture only; no live updater, policy, journal or reboot command.
set -Eeuo pipefail
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
ulimit -c 0
for compiler in g++ clang++; do
 for sanitize in 0 1; do
  flags=(-std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror)
  if [[ $sanitize == 1 ]];then flags+=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer);fi
  "$compiler" "${flags[@]}" "$owner/tests/boot-health-report.cpp" -o "$work/check"
  "$work/check"
  printf 'Actual boot-health observer passed:%s/sanitize%s; physical boot acceptance remains separate.\n' "$compiler" "$sanitize"
 done
done
