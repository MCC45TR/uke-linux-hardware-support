#!/usr/bin/env bash
# Host-only checks of the actual VA dimension-query entrypoints; no device access.
set -Eeuo pipefail
root=$(realpath "${1:?Pass the prepared backend source}")
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
cat > "$work/check.cpp" <<'CPP'
#include "internal.hpp"
#include <cassert>
namespace irisva {
bool vpp_supported(VAProfile, VAEntrypoint) { return false; }
CPP
for fn in guard decode_supported supported encode_supported profile_limits encode_attribute profile_format profile_fourcc; do
    awk -v name="$fn" '$0 ~ "^(template|static).*[^[:alnum:]_]" name "\\(" {emit=1}
        emit {print} emit && /^}/ {exit}' "$root/src/backend.cpp" >> "$work/check.cpp"
done
cat >> "$work/check.cpp" <<'CPP'
#define API(name, args, ...) static VAStatus name args { \
    return guard(ctx, [&](Driver &d) -> VAStatus { __VA_ARGS__ }); }
CPP
for fn in get_attributes query_surfaces; do
    awk -v name="$fn" '
        !emit && $0 ~ "^[[:space:]]*" name "," {print previous; emit=1}
        emit && (/^API/ || /^\/\/ Normalize/) {exit}
        emit {print}
        {previous=$0}' "$root/src/backend.cpp" >> "$work/check.cpp"
done
cat >> "$work/check.cpp" <<'CPP'
}
int main() {
    using namespace irisva;
    Driver driver;
    driver.decoder_profiles.emplace(VAProfileHEVCMain10, FrameSize{96,96,5120,2880});
    driver.encoder_profiles.emplace(VAProfileHEVCMain10, FrameSize{96,96,4096,4096});
    driver.encoder_profiles.emplace(VAProfileH264High, FrameSize{128,128,4096,4096});
    VADriverContext context{};
    context.pDriverData = &driver;
    for (auto entry : {VAEntrypointVLD, VAEntrypointEncSlice}) {
        VAConfigAttrib attributes[] = {{VAConfigAttribMaxPictureWidth,0},
                                       {VAConfigAttribMaxPictureHeight,0}};
        assert(get_attributes(&context,VAProfileHEVCMain10,entry,attributes,2)==VA_STATUS_SUCCESS);
        assert(attributes[0].value==(entry==VAEntrypointVLD ? 5120u : 4096u));
        assert(attributes[1].value==(entry==VAEntrypointVLD ? 2880u : 4096u));
        driver.configs[1] = {VAProfileHEVCMain10,entry};
        unsigned count=0;
        assert(query_surfaces(&context,1,nullptr,&count)==VA_STATUS_SUCCESS);
        assert(count==(entry==VAEntrypointVLD ? 7u : 6u));
        VASurfaceAttrib surfaces[7]{};
        unsigned small=1;
        assert(query_surfaces(&context,1,surfaces,&small)==VA_STATUS_ERROR_MAX_NUM_EXCEEDED);
        assert(small==count);
        assert(query_surfaces(&context,1,surfaces,&count)==VA_STATUS_SUCCESS);
        assert(surfaces[1].value.value.i==96 && surfaces[2].value.value.i==96);
        assert(unsigned(surfaces[3].value.value.i)==attributes[0].value);
        assert(unsigned(surfaces[4].value.value.i)==attributes[1].value);
    }
    driver.configs[2] = {VAProfileH264High,VAEntrypointEncSlice};
    VASurfaceAttrib surfaces[7]{};
    unsigned count=7;
    assert(query_surfaces(&context,2,surfaces,&count)==VA_STATUS_SUCCESS);
    assert(surfaces[1].value.value.i==128 && surfaces[2].value.value.i==128);
    assert(query_surfaces(&context,99,surfaces,&count)==VA_STATUS_ERROR_INVALID_CONFIG);
}
CPP
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror $(pkg-config --cflags libva) \
    -I"$root/src" "$work/check.cpp" -o "$work/check"
"$work/check"
printf 'VA codec-specific dimension queries: passed (host only)\n'
