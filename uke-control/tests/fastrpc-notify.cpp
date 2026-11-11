// SPDX-License-Identifier: MIT
// Drive the unmodified Fedora listener/stubs. All remote RPCs are synthetic.
#include <fastrpc/remote.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <string>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {
int receiver = -1, events = -1, invokes = 0, notices = 0, passes = 0;
bool fail_register = false, fallback = false, enabled = true;
std::string device;
void require(bool ok, const char *message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
void inspect_notice(int expected) {
    char buffer[64]{};
    for (;;) {
        ssize_t count = recv(receiver, buffer, sizeof(buffer), MSG_DONTWAIT);
        if (count < 0) { require(errno == EAGAIN, "notify receive failed"); break; }
        require(count == 7 && std::memcmp(buffer, "READY=1", 7) == 0,
                "unexpected notification payload");
        ++notices;
    }
    require(notices == expected, "notification boundary or count changed");
}
int invocation() {
    // Real generated registration stubs reach this mock before the shim can
    // notify. No descriptor or DSP ioctl is opened by this fixture.
    inspect_notice(passes && enabled ? 1 : 0);
    ++invokes;
    errno = EDOM;
    return fail_register ? 23 : 0;
}
int opening(const char *name, std::uint64_t *handle) {
    require(name && handle, "invalid mock open");
    if (std::strstr(name, "geteventfd")) {
        require(errno == EDOM, "registration errno changed by integration");
        inspect_notice(enabled ? 1 : 0);
        *handle = static_cast<std::uint64_t>(events);
    } else if (std::strstr(name, "adsp_default_listener1")) {
        if (fallback) return 19;
        *handle = 101;
    } else if (std::strstr(name, "adsp_default_listener")) {
        *handle = 102;
    } else {
        require(std::strstr(name, "attachguestos") ||
                std::strstr(name, "createstaticpd:sensorspd"),
                "unexpected PD request");
        *handle = 103;
    }
    return 0;
}
} // namespace

extern "C" void get_domain_device_names(int, const char **secure,
                                         const char **nonsecure) {
    *secure = device.c_str();
    *nonsecure = device.c_str();
}
extern "C" int remote_handle_open(const char *name, remote_handle *handle) {
    std::uint64_t value = 0;
    const int result = opening(name, &value);
    *handle = static_cast<remote_handle>(value);
    return result;
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
extern "C" int remote_handle_close(remote_handle) { return 0; }
extern "C" int remote_handle64_close(remote_handle64) { return 0; }

int main(int argc, char **argv) {
    require(argc == 3, "usage: fixture CASE DIRECTORY");
    const std::string mode = argv[1], root = argv[2];
    const std::string socket_path = root + "/notify.socket";
    device = root + "/synthetic-device";
    FILE *file = std::fopen(device.c_str(), "wx");
    require(file != nullptr, "create synthetic node");
    std::fclose(file);
    receiver = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    require(receiver >= 0, "create notify receiver");
    sockaddr_un address{}; address.sun_family = AF_UNIX;
    require(socket_path.size() < sizeof(address.sun_path), "socket path too long");
    std::memcpy(address.sun_path, socket_path.c_str(), socket_path.size() + 1);
    require(bind(receiver, reinterpret_cast<sockaddr *>(&address),
                 sizeof(address)) == 0, "bind notify receiver");
    require(setenv("NOTIFY_SOCKET", socket_path.c_str(), 1) == 0, "set socket");
    require(setenv("UKE_FASTRPC_NOTIFY", "listener-v1", 1) == 0, "set scope");
    enabled = mode != "disabled" && mode != "wrong-scope" && mode != "no-socket";
    if (mode == "disabled") unsetenv("UKE_FASTRPC_NOTIFY");
    if (mode == "wrong-scope") setenv("UKE_FASTRPC_NOTIFY", "other", 1);
    if (mode == "no-socket") unsetenv("NOTIFY_SOCKET");
    if (mode == "missing-library") {
        using Register = int (*)();
        auto entry = reinterpret_cast<Register>(dlsym(RTLD_DEFAULT,
                                            "adsp_default_listener_register"));
        require(entry && entry() == -ENOSYS && errno == ENOSYS,
                "unloaded original library must fail without READY");
        inspect_notice(0);
    } else {
        void *library = dlopen("libadsp_default_listener.so.1", RTLD_NOW);
        require(library != nullptr, dlerror());
        require(dlsym(RTLD_DEFAULT, "adsp_default_listener_start") == nullptr,
                "fixture no longer covers the daemon RTLD_LOCAL scope");
        using Start = int (*)(int, char **);
        auto start = reinterpret_cast<Start>(dlsym(library,
                                             "adsp_default_listener_start"));
        require(start != nullptr, "missing original Fedora entry");
        fail_register = mode == "failure";
        fallback = mode == "fallback";
        char program[] = "adsprpcd", sensors[] = "sensorspd", adsp[] = "adsp";
        char *args[] = {program, sensors, adsp, nullptr};
        const int arguments = mode == "root" || mode == "repeat" ? 1 : 3;
        const int repeats = mode == "repeat" ? 2 : 1;
        for (int i = 0; i < repeats; ++i) {
            events = eventfd(1, EFD_CLOEXEC);
            require(events >= 0, "synthetic eventfd");
            int result = start(arguments, args);
            require(result == (fail_register ? 23 : 0), "original return changed");
            inspect_notice(enabled && !fail_register ? 1 : 0);
            close(events); events = -1; ++passes;
        }
        require(invokes == repeats, "real registration stub was not exercised");
        dlclose(library);
    }
    close(receiver);
    unlink(socket_path.c_str()); unlink(device.c_str());
    std::printf("Fedora listener case %s passed; synthetic RPC only.\n", mode.c_str());
}
