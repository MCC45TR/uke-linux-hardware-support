#!/usr/bin/env bash
# Host-only actual allocator checks; no GPU or codec qualification.
set -Eeuo pipefail
root=$(realpath "${1:?Pass the prepared backend source}")
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
cat > "$work/check.cpp" <<'CPP'
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <vector>
#include <fcntl.h>
#include <xf86drm.h>
#include <msm_drm.h>
#ifndef MSM_BO_CACHED_COHERENT
#define MSM_BO_CACHED_COHERENT 0x080000
#endif
extern "C" int irisva_msm_allocate(int, uint64_t, int *);
extern "C" int irisva_msm_allocate_wc(int, uint64_t);
static std::vector<unsigned> flags;
static int error, handles, fds;
static bool coherent_only, export_error, close_error;
static void reset() { flags.clear(); error=handles=fds=0; coherent_only=export_error=close_error=false; }
extern "C" int __wrap_drmIoctl(int fd, unsigned long request, void *arg) {
    assert(fd==9);
    if (request==DRM_IOCTL_MSM_GEM_NEW) {
        auto *p=static_cast<drm_msm_gem_new *>(arg);
        assert(p->size==4096); flags.push_back(p->flags);
        if (error && (!coherent_only || p->flags==MSM_BO_CACHED_COHERENT)) { errno=error; return -1; }
        p->handle=7; return 0;
    }
    assert(request==DRM_IOCTL_GEM_CLOSE && static_cast<drm_gem_close *>(arg)->handle==7);
    ++handles; if(close_error) {errno=EINVAL;return -1;} return 0;
}
extern "C" int __wrap_drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t mode, int *out) {
    assert(fd==9 && handle==7 && mode==(DRM_CLOEXEC|DRM_RDWR));
    if(export_error) {errno=EIO;return -1;} *out=123;return 0;
}
extern "C" int __wrap_close(int fd) {assert(fd==123);++fds;return 0;}
int main() {
    int coherent;
    reset(); assert(irisva_msm_allocate(9,4096,&coherent)==123 && coherent==1);
    assert(flags==std::vector<unsigned>{MSM_BO_CACHED_COHERENT} && handles==1 && fds==0);
    for(int e:{EINVAL,EOPNOTSUPP}) {
        reset();error=e;coherent_only=true;
        assert(irisva_msm_allocate(9,4096,&coherent)==123 && coherent==0);
        assert((flags==std::vector<unsigned>{MSM_BO_CACHED_COHERENT,MSM_BO_WC}));
    }
    reset(); assert(irisva_msm_allocate_wc(9,4096)==123);
    assert(flags==std::vector<unsigned>{MSM_BO_WC} && handles==1);
    for(int e:{ENOMEM,EACCES}) {
        reset();error=e;assert(irisva_msm_allocate(9,4096,&coherent)==-1 && errno==e);
        assert(flags.size()==1 && !handles && !fds);
    }
    reset();error=EINVAL;assert(irisva_msm_allocate_wc(9,4096)==-1 && flags.size()==1);
    reset();export_error=true;assert(irisva_msm_allocate_wc(9,4096)==-1 && errno==EIO && handles==1 && !fds);
    reset();close_error=true;assert(irisva_msm_allocate_wc(9,4096)==-1 && handles==1 && fds==1);
}
CPP
drm_cflags=$(pkg-config --cflags libdrm)
read -ra cflags <<< "$drm_cflags"
"${CC:-cc}" -O2 -Wall -Wextra -Werror "${cflags[@]}" -c "$root/src/msm.c" -o "$work/msm.o"
"${CXX:-c++}" -std=c++17 -fno-operator-names -O2 -Wall -Wextra -Werror "${cflags[@]}" "$work/check.cpp" "$work/msm.o" -Wl,--wrap=drmIoctl,--wrap=drmPrimeHandleToFD,--wrap=close -o "$work/check"
"$work/check"
printf '%s\n' 'Shared-surface WC and private-coherent allocator/error checks: passed (host API mocks only)'
