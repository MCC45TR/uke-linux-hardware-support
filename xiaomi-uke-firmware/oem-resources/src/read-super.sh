#!/bin/bash
# SPDX-License-Identifier: MIT
# Format reference: AOSP liblp metadata_format.h, Apache-2.0, blob 8d77097ed67b047603d4571e514730ca98b63cf8.
set -Eeuo pipefail
export LC_ALL=C
geometry=${1:?geometry prefix required}; metadata=${2:?metadata copy required}
expected_bytes=${3:?expected source capacity required}
u32() { od --endian=little -An -tu4 -j "$2" -N 4 "$1" | tr -d ' \n'; }
u16() { od --endian=little -An -tu2 -j "$2" -N 2 "$1" | tr -d ' \n'; }
u64() { od --endian=little -An -tu8 -j "$2" -N 8 "$1" | tr -d ' \n'; }
hex() { od -An -v -tx1 -j "$2" -N "$3" "$1" | tr -d ' \n'; }
slice() { dd if="$1" bs=65536 skip="$2" count="$3" iflag=skip_bytes,count_bytes status=none; }
digest() { sha256sum | cut -d ' ' -f 1; }
fail() { printf '%s\n' "$*" >&2; exit 1; }
test "$(u32 "$geometry" 4096)" = 1634485351 || fail geometry_magic
test "$(u32 "$geometry" 4100)" = 52 || fail geometry_size
gsha=$({ slice "$geometry" 4096 8; head -c 32 /dev/zero; slice "$geometry" 4136 12; } | digest)
test "$gsha" = "$(hex "$geometry" 4104 32)" || fail geometry_checksum
cmp -s <(slice "$geometry" 4096 52) <(slice "$geometry" 8192 52) || fail geometry_copy_mismatch
maximum=$(u32 "$geometry" 4136); slots=$(u32 "$geometry" 4140)
((maximum>=512 && maximum<=1048576 && maximum%512==0 && slots>=1 && slots<=4)) || fail geometry_bounds
test "$(u32 "$metadata" 0)" = 1095520304 || fail metadata_magic
test "$(u16 "$metadata" 4)" = 10 || fail major_version
minor=$(u16 "$metadata" 6); hs=$(u32 "$metadata" 8); ts=$(u32 "$metadata" 44)
((minor<=2 && (hs==128 || hs==256) && hs+ts<=maximum && hs+ts<=$(stat -c %s "$metadata"))) || fail metadata_bounds
hsha=$({ slice "$metadata" 0 12; head -c 32 /dev/zero; slice "$metadata" 44 "$((hs-44))"; } | digest)
test "$hsha" = "$(hex "$metadata" 12 32)" || fail header_checksum
test "$(slice "$metadata" "$hs" "$ts" | digest)" = "$(hex "$metadata" 48 32)" || fail table_checksum
po=$(u32 "$metadata" 80); pc=$(u32 "$metadata" 84); pe=$(u32 "$metadata" 88)
eo=$(u32 "$metadata" 92); ec=$(u32 "$metadata" 96); ee=$(u32 "$metadata" 100)
go=$(u32 "$metadata" 104); gc=$(u32 "$metadata" 108); ge=$(u32 "$metadata" 112)
bo=$(u32 "$metadata" 116); bc=$(u32 "$metadata" 120); be=$(u32 "$metadata" 124)
((pc<=128 && ec<=1024 && gc<=32 && bc==1 && pe==52 && ee==24 && ge==48 && be==64)) || fail table_layout
((po+pc*pe<=ts && eo+ec*ee<=ts && go+gc*ge<=ts && bo+bc*be<=ts)) || fail descriptor_bounds
bs=$(u64 "$metadata" "$((hs+bo+16))"); first=$(u64 "$metadata" "$((hs+bo))")
((bs>=1048576 && bs<=1099511627776 && first>0 && first<bs/512)) || fail source_size
[[ $expected_bytes =~ ^[0-9]+$ ]] && test "$bs" = "$expected_bytes" || fail source_capacity_mismatch
((first*512>=12288+maximum*slots*2)) || fail metadata_extent_overlap
printf 'Verified geometry/checksums; metadata version 10.%s; slots=%s; partitions=%s; extents=%s; source_bytes=%s\n' "$minor" "$slots" "$pc" "$ec" "$bs" >&2
for ((i=0;i<pc;i++)); do
    off=$((hs+po+i*pe)); name=$(slice "$metadata" "$off" 36 | tr -d '\0')
    [[ $name =~ ^[a-zA-Z0-9_-]{1,36}$ ]] || fail partition_name
    flags=$(u32 "$metadata" "$((off+36))"); ix=$(u32 "$metadata" "$((off+40))"); n=$(u32 "$metadata" "$((off+44))"); group=$(u32 "$metadata" "$((off+48))")
    ((ix+n<=ec && group<gc && (flags&~15)==0)) || fail partition_bounds
    total=0; targets=''
    for ((j=0;j<n;j++)); do
        eoff=$((hs+eo+(ix+j)*ee)); sectors=$(u64 "$metadata" "$eoff"); type=$(u32 "$metadata" "$((eoff+8))")
        sector=$(u64 "$metadata" "$((eoff+12))"); source=$(u32 "$metadata" "$((eoff+20))")
        ((sectors>0 && sectors<=bs/512 && source==0)) || fail extent_bounds
        if ((type==0)); then
            ((sector>=first && sector<=bs/512-sectors)) || fail extent_bounds
            targets+="${targets:+,}$sector:$sectors"
        elif ((type==1 && sector==0)); then targets+="${targets:+,}zero:$sectors"
        else fail unsupported_extent; fi
        total=$((total+sectors*512))
    done
    printf '%s\t%s\t%s\t%s\t%s\n' "$name" "$flags" "$n" "$total" "$targets"
done
