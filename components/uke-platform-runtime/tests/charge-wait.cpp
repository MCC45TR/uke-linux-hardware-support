// SPDX-License-Identifier: MIT
// Actual helper wait implementation; no device, firmware or Control main call.
#define main uke_charge_unused_main
#include "../src/battery/control-main.cpp"
#undef main
#include <cassert>
#include <sys/wait.h>

extern "C" int __real_ppoll(pollfd *, nfds_t, const timespec *, const sigset_t *);
extern "C" int __real_pthread_sigmask(int, const sigset_t *, sigset_t *);
namespace {
enum class Fault { None, EnterMask, RestoreMask, Poll, UnexpectedPoll, Interrupted, PendingTerm, PendingInt };
Fault fault = Fault::None;
unsigned polls = 0, blocks = 0, restores = 0;
void reset(Fault value = Fault::None) { fault = value; polls = blocks = restores = 0; stopping = 0; }
sigset_t mask() { sigset_t result{}; assert(!__real_pthread_sigmask(SIG_SETMASK, nullptr, &result)); return result; }
void same_mask(const sigset_t &expected) {
    const auto observed = mask();
    for (int signal = 1; signal < NSIG; ++signal)
        assert(sigismember(&expected, signal) == sigismember(&observed, signal));
}
void error(const std::string &expected) {
    bool caught = false;
    try { wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(5)); }
    catch (const std::runtime_error &e) { caught = true; assert(e.what() == expected); }
    assert(caught);
}
uint64_t cpu_ns() {
    timespec t{}; assert(!clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t));
    return uint64_t(t.tv_sec) * 1000000000ULL + uint64_t(t.tv_nsec);
}
}
extern "C" int __wrap_pthread_sigmask(int how, const sigset_t *set, sigset_t *old) {
    if (set && how == SIG_BLOCK) {
        ++blocks;
        if (fault == Fault::EnterMask) return EINVAL;
        const int result = __real_pthread_sigmask(how, set, old);
        if (!result && (fault == Fault::PendingTerm || fault == Fault::PendingInt))
            assert(!kill(getpid(), fault == Fault::PendingTerm ? SIGTERM : SIGINT));
        return result;
    }
    if (set && how == SIG_SETMASK) {
        ++restores;
        const int result = __real_pthread_sigmask(how, set, old);
        // Inject the error report after restoring so the test cannot leak a mask.
        return result ? result : fault == Fault::RestoreMask ? EIO : 0;
    }
    return __real_pthread_sigmask(how, set, old);
}
extern "C" int __wrap_ppoll(pollfd *fds, nfds_t count, const timespec *timeout, const sigset_t *signals) {
    ++polls; assert(!fds && !count && timeout && signals);
    assert(!sigismember(signals, SIGTERM) && !sigismember(signals, SIGINT));
    if (fault == Fault::Poll) { errno = EIO; return -1; }
    if (fault == Fault::UnexpectedPoll) return 1;
    if (fault == Fault::Interrupted && polls == 1) { errno = EINTR; return -1; }
    return __real_ppoll(fds, count, timeout, signals);
}
int main() {
    signal(SIGTERM, stop); signal(SIGINT, stop);
    const auto original = mask();
    sigset_t extra{}; sigemptyset(&extra); sigaddset(&extra, SIGUSR2);
    assert(!__real_pthread_sigmask(SIG_BLOCK, &extra, nullptr));
    const auto expected = mask();
    reset(); stopping = 1;
    wait_until(std::chrono::steady_clock::now() + std::chrono::seconds(2));
    assert(!polls && blocks == 1 && restores == 1); same_mask(expected);
    reset(); wait_until(std::chrono::steady_clock::now() - std::chrono::seconds(1));
    assert(!polls && blocks == 1 && restores == 1); same_mask(expected);
    reset(); const auto start = std::chrono::steady_clock::now(); const auto cpu = cpu_ns();
    wait_until(start + std::chrono::seconds(2));
    const auto elapsed = std::chrono::steady_clock::now() - start;
    assert(elapsed >= std::chrono::seconds(2) && polls == 1 && cpu_ns() - cpu < 200000000ULL);
    same_mask(expected);
    reset(Fault::Interrupted); const auto noise_start = std::chrono::steady_clock::now();
    wait_until(noise_start + std::chrono::milliseconds(30));
    assert(polls == 2 && std::chrono::steady_clock::now() - noise_start >= std::chrono::milliseconds(30));
    same_mask(expected);
    for (const auto value : {Fault::PendingTerm, Fault::PendingInt}) {
        reset(value); const auto pending_start = std::chrono::steady_clock::now();
        wait_until(pending_start + std::chrono::seconds(3));
        assert(stopping && polls == 1 && std::chrono::steady_clock::now() - pending_start < std::chrono::seconds(1));
        same_mask(expected);
    }
    reset(); const auto parent = getpid(); const auto child = fork(); assert(child >= 0);
    if (!child) { usleep(30000); _exit(kill(parent, SIGTERM) ? 1 : 0); }
    const auto stop_start = std::chrono::steady_clock::now();
    wait_until(stop_start + std::chrono::seconds(3));
    assert(stopping && std::chrono::steady_clock::now() - stop_start < std::chrono::seconds(1));
    int status = 0; assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    same_mask(expected);
    reset(Fault::EnterMask); error("control_wait_mask_errno_" + std::to_string(EINVAL));
    assert(!polls && !restores); same_mask(expected);
    reset(Fault::Poll); error("control_wait_poll_errno_" + std::to_string(EIO));
    assert(polls == 1 && restores == 1); same_mask(expected);
    reset(Fault::UnexpectedPoll); error("control_wait_poll_errno_" + std::to_string(EPROTO));
    assert(polls == 1 && restores == 1); same_mask(expected);
    reset(Fault::RestoreMask); error("control_wait_restore_errno_" + std::to_string(EIO));
    assert(polls == 1 && restores == 1); same_mask(expected);
    assert(!__real_pthread_sigmask(SIG_SETMASK, &original, nullptr));
    std::cout << "Actual charge wait: one two-second idle wait, pending/delayed stop, EINTR deadline, mask retention and syscall errors; no device operation.\n";
}
