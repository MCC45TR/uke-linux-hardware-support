// SPDX-License-Identifier: MIT
// Execute the Control worker against original Fedora listener/stubs.
#include <array>
#include <cerrno>
#include <cstddef>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <poll.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

namespace {
void require(bool ok, const char *message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
void no_device_io() {
    const sock_filter rules[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_ioctl, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };
    sock_fprog program{static_cast<unsigned short>(std::size(rules)),
                       const_cast<sock_filter *>(rules)};
    require(prctl(PR_SET_NO_NEW_PRIVS, 1L, 0L, 0L, 0L) == 0, "set no-new-privileges");
    require(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) == 0, "deny ioctls");
    errno = 0;
    require(ioctl(-1, 0) == -1 && errno == EPERM, "ioctl denial missing");
}
}
int main(int argc, char **argv) {
    require(argc == 6, "usage: CASE DIRECTORY WORKER ADAPTER MOCKS");
    const std::string mode = argv[1], root = argv[2], worker = argv[3],
                      adapter = argv[4], mocks = argv[5];
    const std::string socket_path = root + "/notify", device = root + "/device";
    int fd = open(device.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    require(fd >= 0, "synthetic device file"); close(fd);
    const int receiver = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    require(receiver >= 0, "notify receiver");
    sockaddr_un address{}; address.sun_family = AF_UNIX;
    require(socket_path.size() < sizeof(address.sun_path), "notify path length");
    std::memcpy(address.sun_path, socket_path.c_str(), socket_path.size() + 1);
    require(bind(receiver, reinterpret_cast<sockaddr *>(&address),
                 sizeof(address)) == 0, "bind notify receiver");
    int reports[2]{};
    require(pipe2(reports, O_CLOEXEC) == 0, "mock report pipe");
    no_device_io();
    const pid_t child = fork();
    require(child >= 0, "fork worker");
    if (child == 0) {
        if (prctl(PR_SET_PDEATHSIG, SIGKILL) != 0) _exit(90);
        close(reports[0]);
        if (fcntl(reports[1], F_SETFD, 0) != 0) _exit(90);
        const std::string report_fd = std::to_string(reports[1]);
        const std::string preload = mode == "missing-adapter" ? mocks : adapter + ":" + mocks;
        setenv("LD_PRELOAD", preload.c_str(), 1);
        setenv("UKE_FASTRPC_TEST_REPORT_FD", report_fd.c_str(), 1);
        setenv("UKE_FASTRPC_TEST_DEVICE", device.c_str(), 1);
        setenv("UKE_FASTRPC_NOTIFY", "listener-v1", 1);
        setenv("NOTIFY_SOCKET", socket_path.c_str(), 1);
        unsetenv("UKE_FASTRPC_TEST_FAILURE"); unsetenv("UKE_FASTRPC_TEST_FALLBACK");
        if (mode == "failure") setenv("UKE_FASTRPC_TEST_FAILURE", "1", 1);
        if (mode == "fallback") setenv("UKE_FASTRPC_TEST_FALLBACK", "1", 1);
        if (mode == "unscoped") unsetenv("UKE_FASTRPC_NOTIFY");
        if (mode == "bad-socket") setenv("NOTIFY_SOCKET", "relative-path", 1);
        const char *role = mode == "root" ? "root" : "sensors";
        if (mode == "invalid") role = "invalid";
        execl(worker.c_str(), worker.c_str(), role, static_cast<char *>(nullptr));
        _exit(90);
    }
    close(reports[1]);
    const int pidfd = static_cast<int>(syscall(SYS_pidfd_open, child, 0));
    require(pidfd >= 0, "pidfd worker supervision");
    pollfd event{pidfd, POLLIN, 0};
    const int waited = poll(&event, 1, 5000);
    if (waited <= 0) { kill(child, SIGKILL); waitpid(child, nullptr, 0); }
    require(waited == 1 && (event.revents & POLLIN), "worker did not end one session");
    int status = 0;
    require(waitpid(child, &status, 0) == child && WIFEXITED(status), "worker exit");
    const bool refused = mode == "invalid" || mode == "unscoped" ||
                         mode == "bad-socket" || mode == "missing-adapter";
    const int expected = mode == "invalid" ? EX_USAGE : refused ? EX_CONFIG : EX_TEMPFAIL;
    require(WEXITSTATUS(status) == expected, "worker status policy changed");
    std::array<char,64> observations{};
    const ssize_t length = read(reports[0], observations.data(), observations.size());
    require(length >= 0, "mock report read");
    const std::string sequence(observations.data(), static_cast<std::size_t>(length));
    if (refused) require(sequence.empty(), "refusal reached a PD");
    else {
        const char pd = mode == "root" ? 'R' : 'S';
        require(!sequence.empty() && sequence[0] == pd, "wrong original PD");
        require(sequence.find(pd, 1) == std::string::npos, "worker retried inside one PID");
        require(sequence.find('I') != std::string::npos &&
                sequence.find('I', sequence.find('I') + 1) == std::string::npos,
                "registration count changed");
        require(sequence.back() == 'C', "original listener was not closed");
    }
    std::array<char,64> notice{};
    const ssize_t bytes = recv(receiver, notice.data(), notice.size(), MSG_DONTWAIT);
    const bool ready = !refused && mode != "failure";
    if (ready) require(bytes == 7 && std::memcmp(notice.data(), "READY=1", 7) == 0,
                       "registered worker READY missing");
    else require(bytes == -1 && errno == EAGAIN, "false worker READY");
    require(recv(receiver, notice.data(), notice.size(), MSG_DONTWAIT) == -1 &&
            errno == EAGAIN, "duplicate worker READY");
    close(reports[0]); close(receiver); close(pidfd);
    unlink(socket_path.c_str()); unlink(device.c_str());
    std::printf("Original Fedora session worker case %s passed;one-session exit and no device ioctl.\n", mode.c_str());
}
