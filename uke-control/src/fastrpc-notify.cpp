// SPDX-License-Identifier: MIT
// Session integration for the original Fedora FastRPC listener. No DSP calls.
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <systemd/sd-daemon.h>

namespace {
std::atomic_flag notified = ATOMIC_FLAG_INIT;

template <typename Function> Function original(const char *symbol) noexcept {
    // adsprpcd loads this library RTLD_LOCAL. RTLD_NEXT from a preload cannot
    // see that scope. Obtain only the already loaded, versioned Fedora object.
    void *handle = dlopen("libadsp_default_listener.so.1", RTLD_NOW | RTLD_NOLOAD);
    if (!handle)
        return nullptr;
    void *address = dlsym(handle, symbol);
    dlclose(handle);
    return reinterpret_cast<Function>(address);
}

void ready_after_registration(int result) noexcept {
    const char *mode = std::getenv("UKE_FASTRPC_NOTIFY");
    if (result != 0 || !mode || std::strcmp(mode, "listener-v1") != 0)
        return;
    const char *socket = std::getenv("NOTIFY_SOCKET");
    if (!socket || (socket[0] != '/' && socket[0] != '@'))
        return;
    if (notified.test_and_set(std::memory_order_acq_rel))
        return;
    if (sd_notify(0, "READY=1") <= 0)
        notified.clear(std::memory_order_release);
}
} // namespace

extern "C" __attribute__((visibility("default")))
int adsp_default_listener_register() noexcept {
    using Function = int (*)();
    Function next = original<Function>("adsp_default_listener_register");
    if (!next || next == &adsp_default_listener_register) {
        errno = ENOSYS;
        return -ENOSYS;
    }
    const int result = next();
    const int saved_errno = errno;
    ready_after_registration(result);
    errno = saved_errno;
    return result;
}

extern "C" __attribute__((visibility("default")))
int adsp_default_listener1_register(std::uint64_t listener) noexcept {
    using Function = int (*)(std::uint64_t);
    Function next = original<Function>("adsp_default_listener1_register");
    if (!next || next == &adsp_default_listener1_register) {
        errno = ENOSYS;
        return -ENOSYS;
    }
    const int result = next(listener);
    const int saved_errno = errno;
    ready_after_registration(result);
    errno = saved_errno;
    return result;
}
