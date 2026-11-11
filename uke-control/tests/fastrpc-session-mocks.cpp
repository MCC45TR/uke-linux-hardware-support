// SPDX-License-Identifier: MIT
// Test-only remote backend for the original Fedora listener.
#include <fastrpc/remote.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/eventfd.h>
#include <unistd.h>

namespace {
void report(char event) {
    const char *text = std::getenv("UKE_FASTRPC_TEST_REPORT_FD");
    if (!text) _exit(91);
    const int fd = std::atoi(text);
    if (write(fd, &event, 1) != 1) _exit(92);
}
int opening(const char *name, std::uint64_t *handle) {
    if (!name || !handle) _exit(93);
    if (std::strstr(name, "geteventfd")) {
        int events = eventfd(1, EFD_CLOEXEC);
        if (events < 0) _exit(94);
        *handle = static_cast<std::uint64_t>(events);
        report('E');
    } else if (std::strstr(name, "adsp_default_listener1")) {
        if (std::getenv("UKE_FASTRPC_TEST_FALLBACK")) return 19;
        *handle = 101;
    } else if (std::strstr(name, "adsp_default_listener")) {
        *handle = 102;
    } else if (std::strstr(name, "attachguestos")) {
        *handle = 103;
        report('R');
    } else if (std::strstr(name, "createstaticpd:sensorspd")) {
        *handle = 104;
        report('S');
    } else _exit(95);
    return 0;
}
int invocation() {
    report('I');
    errno = EDOM;
    return std::getenv("UKE_FASTRPC_TEST_FAILURE") ? 23 : 0;
}
}
extern "C" void get_domain_device_names(int, const char **secure,
                                        const char **nonsecure) {
    const char *path = std::getenv("UKE_FASTRPC_TEST_DEVICE");
    if (!path) _exit(96);
    *secure = path; *nonsecure = path;
}
extern "C" int remote_handle_open(const char *name, remote_handle *handle) {
    std::uint64_t result = 0;
    const int status = opening(name, &result);
    *handle = static_cast<remote_handle>(result);
    return status;
}
extern "C" int remote_handle64_open(const char *name, remote_handle64 *handle) {
    return opening(name, handle);
}
extern "C" int remote_handle_invoke(remote_handle, std::uint32_t, remote_arg *) {
    return invocation();
}
extern "C" int remote_handle64_invoke(remote_handle64, std::uint32_t, remote_arg *) {
    return invocation();
}
extern "C" int remote_handle_close(remote_handle) { report('C'); return 0; }
extern "C" int remote_handle64_close(remote_handle64) { report('C'); return 0; }
