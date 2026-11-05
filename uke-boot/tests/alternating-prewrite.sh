#!/usr/bin/env bash
# COPR-only actual libfdisk pre-write/reload/selection fixtures, regular files.
set -Eeuo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
ulimit -c 0
[[ $(grep -Fc 'block_inactive(running,target,!saved.baseline_approved)' src/userdata-update.hpp) == 1 ]]
for compiler in g++ clang++;do
 for sanitized in 0 1;do CXX=$compiler SANITIZE=$sanitized bash tests/slot-control.sh;done
done
work=$(mktemp -d);trap 'rm -rf -- "$work"' EXIT
cp -a src tests "$work/"
awk '
 index($0,"need(((held.attrs&successful)") {
  print "        need((held.attrs&successful) && !(held.attrs&unbootable),";changed++;next
 }
 {print} END {if(changed!=1)exit 1}
' src/slot-control.hpp > "$work/src/slot-control.hpp"
for compiler in g++ clang++;do
 set +e
 (cd "$work"; CXX=$compiler SANITIZE=0 bash tests/slot-control.sh) > "$work/$compiler.original.log" 2>&1
 status=$?
 set -e
 [[ $status == 134 ]] || { cat "$work/$compiler.original.log" >&2;exit 1; }
 printf 'Original accepted-only pre-write guard fails actual checkpoint barrier: %s/status134.\n' "$compiler"
done
printf 'Actual pre-write/reload/final selection: both directions,512/4096 sectors,4 repeated checkpoints,4 compiler/sanitizer modes; defaults still refuse unapproved boot.\n'
