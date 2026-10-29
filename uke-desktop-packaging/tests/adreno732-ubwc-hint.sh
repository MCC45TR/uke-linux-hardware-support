#!/usr/bin/env bash
# Host-only fixture for the exact shared Mesa function; opens no GPU.
set -euo pipefail
project=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
source_dir=${1:?Usage: adreno732-ubwc-hint.sh extracted-Mesa-source-directory [--patched]}
[[ $# == 1 || ( $# == 2 && $2 == --patched ) ]]
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT
mkdir -p "$tmp/src/freedreno/common"
cp -- "$source_dir/src/freedreno/common/freedreno_dev_info.c" "$tmp/src/freedreno/common/"
if [[ $# == 1 ]]; then
    patch --batch --fuzz=0 -d "$tmp" -p1 < "$project/packaging/mesa/0004-freedreno-adreno732-ubwc-hint.patch"
fi
cat > "$tmp/check.cpp" <<'CPP'
#include <cassert>
#include <cstdint>
#include <initializer_list>
struct fd_dev_id { uint64_t chip_id; };
struct fd_dev_info {
    struct { bool enable_tp_ubwc_flag_hint; unsigned stable; } props;
    unsigned num_ccu;
};
static const fd_dev_info original{{true, 77}, 3};
static bool available = true, debug_override;
static unsigned debug_calls;
static const fd_dev_info *fd_dev_info_raw(const fd_dev_id *) {
    return available ? &original : nullptr;
}
static void fd_dev_info_apply_dbg_options(fd_dev_info *info) {
    ++debug_calls;
    if (debug_override) info->props.enable_tp_ubwc_flag_hint = true;
}
CPP
awk '/^fd_dev_info\(const struct fd_dev_id \*id\)/ {copy=1; print "struct fd_dev_info"} copy {print} copy && /^}/ {exit}' \
    "$tmp/src/freedreno/common/freedreno_dev_info.c" >> "$tmp/check.cpp"
cat >> "$tmp/check.cpp" <<'CPP'
static void check(uint64_t chip, bool expected) {
    const fd_dev_id id{chip};
    const auto result = fd_dev_info(&id);
    assert(result.props.enable_tp_ubwc_flag_hint == expected);
    assert(result.props.stable == 77 && result.num_ccu == 3);
    assert(original.props.enable_tp_ubwc_flag_hint);
}
int main() {
    for (uint64_t chip : {UINT64_C(0x43030b00), UINT64_C(0xffff43030b00), UINT64_C(0x123443030b00)})
        check(chip, false);
    for (uint64_t chip : {UINT64_C(0xffff43030c00), UINT64_C(0x43030a00), UINT64_C(0x43050a01)})
        check(chip, true);
    debug_override = true;
    check(UINT64_C(0x43030b00), true);
    debug_override = false;
    available = false;
    debug_calls = 0;
    const fd_dev_id unknown{UINT64_C(0x43030b00)};
    const auto absent = fd_dev_info(&unknown);
    assert(!absent.props.enable_tp_ubwc_flag_hint && absent.props.stable == 0 && absent.num_ccu == 0);
    assert(debug_calls == 0);
}
CPP
"${CXX:-c++}" -std=c++20 -Wall -Wextra -Werror "$tmp/check.cpp" -o "$tmp/check"
"$tmp/check"
printf '%s\n' 'Shared Adreno 732 hint fixture passed; full Mesa build and device artifact remain separate'
