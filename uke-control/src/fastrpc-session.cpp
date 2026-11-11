// SPDX-License-Identifier: MIT
// Linux service/session policy only; the original Fedora library owns FastRPC.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <sysexits.h>

int main(int argc, char **argv) {
    if (argc != 2 || (std::strcmp(argv[1], "root") != 0 &&
                      std::strcmp(argv[1], "sensors") != 0)) {
        std::fputs("Usage: uke-fastrpc-session root|sensors\n", stderr);
        return EX_USAGE;
    }
    const char *scope = std::getenv("UKE_FASTRPC_NOTIFY");
    const char *socket = std::getenv("NOTIFY_SOCKET");
    if (!scope || std::strcmp(scope, "listener-v1") != 0 ||
        !socket || (socket[0] != '/' && socket[0] != '@')) {
        std::fputs("FastRPC session requires its scoped notify service.\n", stderr);
        return EX_CONFIG;
    }
    if (!dlsym(RTLD_DEFAULT, "adsp_default_listener_register") ||
        !dlsym(RTLD_DEFAULT, "adsp_default_listener1_register")) {
        std::fputs("FastRPC session notification integration is unavailable.\n", stderr);
        return EX_CONFIG;
    }
    void *library = dlopen("libadsp_default_listener.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        std::fputs("Original Fedora ADSP listener is unavailable.\n", stderr);
        return EX_UNAVAILABLE;
    }
    using Start = int (*)(int, char **);
    auto start = reinterpret_cast<Start>(dlsym(library, "adsp_default_listener_start"));
    if (!start) {
        dlclose(library);
        std::fputs("Original Fedora ADSP listener entry is unavailable.\n", stderr);
        return EX_UNAVAILABLE;
    }
    char program[] = "uke-fastrpc-session";
    char root[] = "rootpd", sensors[] = "sensorspd", domain[] = "adsp";
    char *arguments[] = {program,
        std::strcmp(argv[1], "root") == 0 ? root : sensors, domain, nullptr};
    const int result = start(3, arguments);
    dlclose(library);
    std::fprintf(stderr, "FastRPC %s session ended (status %#x).\n", argv[1], result);
    // A completed listener is no longer ready. Let systemd release dependencies,
    // back off and start a fresh process rather than retry inside this PID.
    return EX_TEMPFAIL;
}
