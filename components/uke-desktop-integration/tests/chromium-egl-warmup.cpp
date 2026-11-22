// SPDX-License-Identifier: MIT
// Diagnostic only: warm driver discovery in Chromium's existing GPU zygote.
#include <cassert>
#include <cstring>

static bool gpu_zygote(int argc, const char* const* argv)
{
    if (argc < 1 || !argv || !argv[0])
        return false;
    const char* basename = std::strrchr(argv[0], '/');
    if (std::strcmp(basename ? basename + 1 : argv[0], "chrome") != 0)
        return false;
    const char* type = nullptr;
    bool unsandboxed_zygote = false;
    for (int i = 1; i < argc && std::strcmp(argv[i], "--") != 0; ++i) {
        if (std::strncmp(argv[i], "--type=", 7) == 0)
            type = argv[i] + 7;
        unsandboxed_zygote |= std::strcmp(argv[i], "--no-zygote-sandbox") == 0;
    }
    return type && std::strcmp(type, "zygote") == 0 && unsandboxed_zygote;
}

#ifdef UKE_WARMUP_SELFTEST
int main()
{
    const char* gpu[] = {"chrome", "--type=zygote", "--no-zygote-sandbox"};
    const char* renderer[] = {"chrome", "--type=zygote"};
    const char* wrong[] = {"chrome", "--type=gpu-process", "--no-zygote-sandbox"};
    const char* lookalike[] = {"chrome", "--type=zygote", "--no-zygote-sandbox=yes"};
    assert(gpu_zygote(3, gpu));
    assert(!gpu_zygote(2, renderer));
    assert(!gpu_zygote(3, wrong));
    assert(!gpu_zygote(3, lookalike));
    assert(!gpu_zygote(1, gpu));
    const char* shell[] = {"bash", "--type=zygote", "--no-zygote-sandbox"};
    const char* duplicate[] = {"chrome", "--type=zygote", "--no-zygote-sandbox", "--type=renderer"};
    const char* terminated[] = {"chrome", "--", "--type=zygote", "--no-zygote-sandbox"};
    assert(!gpu_zygote(3, shell));
    assert(!gpu_zygote(4, duplicate));
    assert(!gpu_zygote(4, terminated));
    assert(!gpu_zygote(0, nullptr));
}
#else
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <dirent.h>
#include <dlfcn.h>
#include <cstdio>
#include <unistd.h>

static int entries(const char* path)
{
    DIR* dir = opendir(path);
    if (!dir)
        return -1;
    int n = 0;
    while (const auto* entry = readdir(dir))
        if (entry->d_name[0] != '.')
            ++n;
    closedir(dir);
    return n;
}

__attribute__((constructor)) static void warmup(int argc, char** argv, char**)
{
    if (!gpu_zygote(argc, argv))
        return;
    const int threads = entries("/proc/self/task"), fds = entries("/proc/self/fd");
    if (threads != 1 || fds < 0)
        _exit(125);
    const char* libraries[] = {
        "/usr/lib64/libEGL.so.1", "/usr/lib64/libgbm.so.1",
        "/usr/lib64/gbm/dri_gbm.so", "/usr/lib64/dri/msm_dri.so"};
    void* egl = nullptr;
    for (const char* library : libraries) {
        void* handle = dlopen(library, RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE);
        if (!handle) {
            dprintf(2, "UKE_EGL_WARMUP: %s: %s\n", library, dlerror());
            _exit(125);
        }
        if (!egl)
            egl = handle;
    }
    const auto query = reinterpret_cast<PFNEGLQUERYSTRINGPROC>(dlsym(egl, "eglQueryString"));
    const auto get_proc = reinterpret_cast<PFNEGLGETPROCADDRESSPROC>(dlsym(egl, "eglGetProcAddress"));
    const auto query_devices = get_proc ? reinterpret_cast<PFNEGLQUERYDEVICESEXTPROC>(get_proc("eglQueryDevicesEXT")) : nullptr;
    EGLint device_count = 0;
    const bool queried = query && query(EGL_NO_DISPLAY, EGL_EXTENSIONS) &&
        query_devices && query_devices(0, nullptr, &device_count) && device_count > 0;
    const int threads_after = entries("/proc/self/task"), fds_after = entries("/proc/self/fd");
    dprintf(2, "UKE_EGL_WARMUP: pid=%ld queried=%d devices=%d threads=%d/%d fds=%d/%d\n",
            static_cast<long>(getpid()), queried, device_count, threads, threads_after, fds, fds_after);
    // Do not hand a threaded or descriptor-leaking zygote to sandbox startup.
    if (!queried || threads != threads_after || fds != fds_after)
        _exit(125);
}
#endif
