// SPDX-License-Identifier: MIT
// Native build fixture only. Original systemd remains unchanged and uninstalled.
// v262 manager_startup creates its runtime directory before applying --root.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <systemd/sd-path.h>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <limits>

extern "C" int sd_path_lookup(std::uint64_t type,const char *suffix,char **ret) {
    if (type!=SD_PATH_SYSTEM_RUNTIME) {
        using Lookup=int (*)(std::uint64_t,const char *,char **);
        auto next=reinterpret_cast<Lookup>(dlsym(RTLD_NEXT,"sd_path_lookup"));
        return next ? next(type,suffix,ret) : -ENOSYS;
    }
    const char *path=std::getenv("UKE_VERIFY_RUNTIME");
    if (!path || path[0]!='/' || !ret)
        return -EINVAL;
    const auto size=std::strlen(path),extra=suffix ? std::strlen(suffix) : 0;
    if (extra>std::numeric_limits<std::size_t>::max()-size-2)
        return -ENOMEM;
    char *value=static_cast<char *>(std::malloc(size+extra+2));
    if (!value)
        return -ENOMEM;
    std::memcpy(value,path,size);
    std::size_t offset=size;
    if (extra) {
        if (value[offset-1]!='/')value[offset++]='/';
        std::memcpy(value+offset,suffix,extra);
        offset+=extra;
    }
    value[offset]=0;
    *ret=value;
    return 0;
}
