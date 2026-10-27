#!/usr/bin/env bash
# Host-only checks of actual Mesa lookup/cache functions; no DRM device opens.
set -euo pipefail
project=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
source_dir=${1:?Usage: egl-device-node-cache.sh Mesa-source-directory [--patched]}
[[ $# == 1 || ( $# == 2 && $2 == --patched ) ]]
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT
mkdir -p "$tmp/candidate/src/egl/main" "$tmp/control/src/egl/main"
cp "$source_dir/src/egl/main/egldevice.c" "$tmp/candidate/src/egl/main/egldevice.c"
patchfile=$project/packaging/mesa/0006-egl-cache-drm-node-identity.patch
if [[ $# == 1 ]]; then patch --batch --forward --fuzz=0 -d "$tmp/candidate" -p1 < "$patchfile"; fi
cp "$tmp/candidate/src/egl/main/egldevice.c" "$tmp/control/src/egl/main/egldevice.c"
patch --batch --reverse --fuzz=0 -d "$tmp/control" -p1 < "$patchfile"
cat > "$tmp/prefix.cpp" <<'CPP'
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#define DRM_NODE_MAX 3
#define DRM_NODE_PRIMARY 0
#define DRM_NODE_RENDER 2
struct drmDevice { int available_nodes; const char* nodes[3]; int identity; };
using drmDevicePtr = drmDevice*;
struct _egl_device {
    _egl_device* Next;
    bool software;
    drmDevicePtr device;
    struct stat node_stat[3];
};
using _EGLDevice = _egl_device;
enum { _EGL_DEVICE_SOFTWARE, _EGL_DEVICE_DRM };
struct { int Mutex; _EGLDevice* DeviceList; } _eglGlobal;
static bool _eglDeviceSupports(_EGLDevice* d, int kind) {
    return kind == _EGL_DEVICE_SOFTWARE ? d->software : !d->software;
}
#define simple_mtx_lock(x) ((void)(x))
#define simple_mtx_unlock(x) ((void)(x))
static int metadata_result = -1, metadata_calls, frees;
static drmDevice metadata{};
static int drmGetDevice2(int, int, drmDevicePtr* d) {
    ++metadata_calls;
    if (metadata_result == 0) *d = &metadata;
    return metadata_result;
}
static bool drmDevicesEqual(drmDevicePtr a, drmDevicePtr b) {
    return a->identity == b->identity;
}
static void drmFreeDevice(drmDevicePtr* d) { ++frees; *d = nullptr; }
CPP
candidate=$tmp/candidate/src/egl/main/egldevice.c
printf 'static bool\n' >> "$tmp/prefix.cpp"
sed -n '/^_eglSameDrmNode(/,/^}/p' "$candidate" >> "$tmp/prefix.cpp"
printf 'static void\n' >> "$tmp/prefix.cpp"
sed -n '/^_eglCacheDrmNodes(/,/^}/p' "$candidate" >> "$tmp/prefix.cpp"
cat > "$tmp/main.cpp" <<'CPP'
int main() {
    const int primary = open("/dev/null", O_RDWR | O_CLOEXEC);
    const int render = open("/dev/zero", O_RDWR | O_CLOEXEC);
    const int alias = dup(primary);
    FILE* regular = tmpfile();
    assert(primary >= 0 && render >= 0 && alias >= 0 && regular);
    drmDevice device{(1 << DRM_NODE_PRIMARY) | (1 << DRM_NODE_RENDER),
                     {"/dev/null", nullptr, "/dev/zero"}, 1};
    _EGLDevice hardware{}, software{};
    software.Next = &hardware; software.software = true;
    hardware.device = &device; _eglGlobal.DeviceList = &software;
    _eglCacheDrmNodes(&hardware, &device);
    const auto saved = hardware.node_stat[DRM_NODE_PRIMARY];
    assert(_eglSameDrmNode(&saved, &saved));
    if (_eglFindDevice(primary, false) != &hardware || metadata_calls != 0)
        return 41;
    assert(_eglFindDevice(render, false) == &hardware && metadata_calls == 0);
    assert(_eglFindDevice(alias, false) == &hardware && metadata_calls == 0);
    for (int field = 0; field < 6; ++field) {
        auto& cached = hardware.node_stat[DRM_NODE_PRIMARY]; cached = saved;
        switch (field) {
        case 0: ++cached.st_dev; break;
        case 1: ++cached.st_ino; break;
        case 2: ++cached.st_rdev; break;
        case 3: ++cached.st_ctim.tv_sec; break;
        case 4: cached.st_ctim.tv_nsec ^= 1; break;
        case 5: cached.st_nlink = 0; break;
        }
        metadata_calls = 0;
        assert(_eglFindDevice(primary, false) == nullptr && metadata_calls == 1);
    }
    hardware.node_stat[DRM_NODE_PRIMARY] = saved;
    hardware.node_stat[DRM_NODE_PRIMARY].st_mode = S_IFREG;
    metadata_calls = 0;
    assert(_eglFindDevice(primary, false) == nullptr && metadata_calls == 1);
    hardware.node_stat[DRM_NODE_PRIMARY] = saved;
    metadata_calls = 0;
    assert(_eglFindDevice(fileno(regular), false) == nullptr && metadata_calls == 1);
    metadata_calls = 0;
    assert(_eglFindDevice(-1, false) == nullptr && metadata_calls == 1);
    metadata_calls = 0;
    assert(_eglFindDevice(-1, true) == &software && metadata_calls == 0);
    device.nodes[DRM_NODE_PRIMARY] = "/nonexistent-uke-egl-node";
    _eglCacheDrmNodes(&hardware, &device);
    metadata_calls = 0;
    assert(_eglFindDevice(primary, false) == nullptr && metadata_calls == 1);
    assert(_eglFindDevice(render, false) == &hardware && metadata_calls == 1);
    metadata_result = 0; metadata.identity = 1; metadata_calls = frees = 0;
    assert(_eglFindDevice(primary, false) == &hardware && metadata_calls == 1 && frees == 1);
    metadata.identity = 2; metadata_calls = frees = 0;
    assert(_eglFindDevice(primary, false) == nullptr && metadata_calls == 1 && frees == 1);
    assert(close(alias) == 0 && close(render) == 0 && close(primary) == 0);
    assert(fclose(regular) == 0);
}
CPP
for variant in candidate control; do
    cat "$tmp/prefix.cpp" > "$tmp/$variant.cpp"
    printf '_EGLDevice*\n' >> "$tmp/$variant.cpp"
    sed -n '/^_eglFindDevice(/,/^}/p' "$tmp/$variant/src/egl/main/egldevice.c" >> "$tmp/$variant.cpp"
    cat "$tmp/main.cpp" >> "$tmp/$variant.cpp"
    "${CXX:-c++}" -std=c++17 -DHAVE_LIBDRM -Wall -Wextra -Werror "$tmp/$variant.cpp" -o "$tmp/$variant-check"
done
"$tmp/candidate-check"
result=0
"$tmp/control-check" || result=$?
[[ $result == 41 ]] || { printf 'Original lookup did not fail the cached-node case: %s\n' "$result" >&2; exit 1; }
printf '%s\n' 'Actual EGL node-cache checks passed; original lookup fails the same cached-node case. No GPU test performed.'
