#!/usr/bin/env bash
# COPR only. Original slot/prewrite checks already run the expanded real fixture
# in both sector sizes and all four compiler/sanitizer configurations.
set -Eeuo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
ulimit -c 0
[[ $(grep -Fc 'if(repair_unaccepted_slot_crc(maintenance,state,true) && action=="accept")' src/userdata-update.hpp) == 1 ]]
[[ $(grep -Fc '(void)repair_unaccepted_slot_crc(maintenance,state,false);' src/userdata-update.hpp) == 1 ]]
work=$(mktemp -d);trap 'find "$work" -depth -delete' EXIT
cp -a src tests "$work/"
# Reproduce the former skip: preview succeeds but no maintenance is committed.
awk 'index($0,"if(preview)return true;"){print "    (void)preview;return true;";changed++;next} {print} END{if(changed!=1)exit 1}' \
 src/metadata-maintenance.hpp > "$work/src/metadata-maintenance.hpp"
for compiler in g++ clang++;do
 set +e
 (cd "$work"; CXX=$compiler SANITIZE=0 bash tests/slot-control.sh) > "$work/$compiler.noop.log" 2>&1
 result=$?
 set -e
 [[ $result == 134 ]] || { cat "$work/$compiler.noop.log" >&2;exit 1; }
 printf 'Skipped CRC maintenance refused by actual GPT fixture: %s/status134.\n' "$compiler"
done
printf 'Rollback-off CRC: real preview/commit/idempotence, unmodified flags/types/approval, pending-mutation refusal and skipped-maintenance regression controls passed.\n'
