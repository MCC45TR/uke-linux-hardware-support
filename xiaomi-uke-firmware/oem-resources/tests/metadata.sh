#!/bin/bash
set -Eeuo pipefail
export LC_ALL=C
root=$(cd -- "$(dirname "$0")/.."; pwd)
for file in "$root"/src/*.sh; do bash -n "$file"; done
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
cd "$stage"
put32() {
 local file=$1 offset=$2 value=$3 i
 for ((i=0;i<4;i++)); do printf '%b' "\\$(printf '%03o' "$(((value>>(8*i))&255))")"; done | dd of="$file" bs=1 seek="$offset" conv=notrunc status=none
}
put_hash() {
 local file=$1 offset=$2 hash=$3 i
 for ((i=0;i<64;i+=2)); do printf '%b' "\\x${hash:i:2}"; done | dd of="$file" bs=1 seek="$offset" conv=notrunc status=none
}
put_text() { printf '%s' "$3" | dd of="$1" bs=1 seek="$2" conv=notrunc status=none; }
dd if=/dev/zero of=geometry bs=12288 count=1 status=none
put32 geometry 4096 1634485351; put32 geometry 4100 52
put32 geometry 4136 4096; put32 geometry 4140 2; put32 geometry 4144 4096
put_hash geometry 4104 "$(dd if=geometry bs=1 skip=4096 count=52 status=none | sha256sum | cut -d' ' -f1)"
dd if=geometry of=backup bs=1 skip=4096 count=52 status=none
dd if=backup of=geometry bs=1 seek=8192 conv=notrunc status=none
dd if=/dev/zero of=metadata bs=4096 count=1 status=none
put32 metadata 0 1095520304; put32 metadata 4 10; put32 metadata 8 128; put32 metadata 44 188
for descriptor in '80 0 1 52' '92 52 1 24' '104 76 1 48' '116 124 1 64'; do
 read -r off start count size <<< "$descriptor"
 put32 metadata "$off" "$start"; put32 metadata "$((off+4))" "$count"; put32 metadata "$((off+8))" "$size"
done
put_text metadata 128 vendor_a; put32 metadata 164 1; put32 metadata 172 1
put32 metadata 180 8192; put32 metadata 192 2048
put_text metadata 204 default; put32 metadata 252 2048
put32 metadata 260 1048576; put32 metadata 268 16777216; put_text metadata 276 super
put_hash metadata 48 "$(dd if=metadata bs=1 skip=128 count=188 status=none | sha256sum | cut -d' ' -f1)"
put_hash metadata 12 "$(head -c128 metadata | sha256sum | cut -d' ' -f1)"
bash "$root/src/read-super.sh" geometry metadata 16777216 > result
printf 'vendor_a\t1\t1\t4194304\t2048:8192\n' > expected
cmp result expected
if bash "$root/src/read-super.sh" geometry metadata 33554432 >/dev/null 2>&1; then exit 1; fi
cp metadata damaged
put_text damaged 128 X
if bash "$root/src/read-super.sh" geometry damaged 16777216 >/dev/null 2>&1; then exit 1; fi
cp metadata damaged
put32 damaged 80 999
if bash "$root/src/read-super.sh" geometry damaged 16777216 >/dev/null 2>&1; then exit 1; fi
put_text geometry 4104 X
if bash "$root/src/read-super.sh" geometry metadata 16777216 >/dev/null 2>&1; then exit 1; fi
printf 'Metadata fixture, geometry/header/table corruption and wrong-capacity refusals passed\n'
