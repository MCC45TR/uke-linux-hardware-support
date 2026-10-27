// SPDX-License-Identifier: MIT
// Process-local AArch64 diagnosis only; no core data or other process is read.
#include <cstdint>
#include <csignal>
#include <fcntl.h>
#include <ucontext.h>
#include <unistd.h>

#if !defined(__aarch64__)
#error Build this diagnostic for AArch64.
#endif

static void address(const char *label, std::uint64_t value)
{
    char line[96];
    unsigned n = 0;
    for (const char *p = label; *p && n < 70; ++p) line[n++] = *p;
    line[n++] = '='; line[n++] = '0'; line[n++] = 'x';
    for (int shift = 60; shift >= 0; shift -= 4)
        line[n++] = "0123456789abcdef"[(value >> shift) & 15];
    line[n++] = '\n';
    if (write(STDERR_FILENO, line, n) < 0) return;
}

static void fault(int signal, siginfo_t *info, void *context)
{
    const auto *state = static_cast<const ucontext_t *>(context);
    address("UKE_FAULT_PC", state->uc_mcontext.pc);
    address("UKE_FAULT_LR", state->uc_mcontext.regs[30]);
    address("UKE_FAULT_ADDRESS", reinterpret_cast<std::uintptr_t>(info->si_addr));

    // Kernel-checked reads stop on invalid or omitted frame-pointer chains.
    int fd = open("/proc/self/mem", O_RDONLY | O_CLOEXEC);
    std::uint64_t fp = state->uc_mcontext.regs[29], frame[2];
    for (unsigned depth = 0; fd >= 0 && depth < 16 && fp && !(fp & 15); ++depth) {
        if (pread(fd, frame, sizeof(frame), static_cast<off_t>(fp)) != sizeof(frame)) break;
        address("UKE_FAULT_FRAME", frame[1]);
        if (frame[0] <= fp || frame[0] - fp > 1024 * 1024) break;
        fp = frame[0];
    }
    if (fd >= 0) close(fd);

    fd = open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
    char buffer[4096];
    for (unsigned block = 0; fd >= 0 && block < 64; ++block) {
        const ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n <= 0 || write(STDERR_FILENO, buffer, static_cast<size_t>(n)) < 0) break;
    }
    if (fd >= 0) close(fd);
    _exit(128 + signal);
}

__attribute__((constructor)) static void install_fault_trace()
{
    struct sigaction action{};
    action.sa_sigaction = fault;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGSEGV, &action, nullptr) != 0) _exit(2);
}
