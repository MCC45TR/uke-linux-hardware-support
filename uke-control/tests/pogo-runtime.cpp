// SPDX-License-Identifier: MIT
#include "../src/pogo-runtime.hpp"
#include <cassert>
#include <chrono>
#include <future>
#include <sys/socket.h>

using namespace uke_pogo_runtime;

template<class F> static void refused(F operation)
{
    bool failed = false;
    try { operation(); } catch (const std::runtime_error&) { failed = true; }
    assert(failed);
}

static int before_poll_stop(struct pollfd* fds, nfds_t count, int timeout)
{
    assert(count == 2 && timeout == -1);
    assert(fds[0].events == (POLLPRI | POLLERR) && fds[1].events == POLLIN);
    // Exercise the old signal-before-poll gap, without an arbitrary sleep.
    assert(::raise(SIGTERM) == 0);
    return ::poll(fds, count, timeout);
}

static int interrupted(struct pollfd*, nfds_t count, int timeout)
{
    assert(count == 2 && timeout == -1);
    errno = EINTR;
    return -1;
}

static int impossible_timeout(struct pollfd*, nfds_t, int timeout)
{
    assert(timeout == -1);
    return 0;
}

int main()
{
    ::alarm(15);
    for (const auto value : {"7.3.1-1-unstable", "7.3.1-12211-unstable", "7.3.3-24-unstable"})
        assert(published_unstable_release(value));
    for (const auto value : {"7.3.3-24", "7.3.3-24-stable", "7.3.2-24-unstable",
                            "7.3.4-24-unstable", "7.3.3-0-unstable", "7.3.3-024-unstable",
                            "7.3.3--unstable", "7.3.3-24x-unstable", "7.3.3-../24-unstable",
                            "7.3.3-24-unstable-extra"})
        assert(!published_unstable_release(value));

    struct sigaction term_before{}, int_before{}, term_after{}, int_after{};
    assert(::sigaction(SIGTERM, nullptr, &term_before) == 0);
    assert(::sigaction(SIGINT, nullptr, &int_before) == 0);
    {
        StopWakeup stop_channel;
        assert(wait_for_bridge_change(-1, stop_channel.descriptor(), before_poll_stop) == Wake::stop);
        assert(stopping.load());
        refused([] { StopWakeup duplicate; });
    }
    assert(stop_descriptor.load() == -1);
    assert(::sigaction(SIGTERM, nullptr, &term_after) == 0);
    assert(::sigaction(SIGINT, nullptr, &int_after) == 0);
    assert(term_before.sa_handler == term_after.sa_handler);
    assert(int_before.sa_handler == int_after.sa_handler);

    {
        StopWakeup stop_channel;
        std::promise<void> entering;
        auto ready = entering.get_future();
        auto waiter = std::async(std::launch::async, [&] {
            entering.set_value();
            return wait_for_bridge_change(-1, stop_channel.descriptor());
        });
        ready.get();
        assert(waiter.wait_for(std::chrono::milliseconds(40)) == std::future_status::timeout);
        assert(::raise(SIGINT) == 0);
        assert(waiter.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
        assert(waiter.get() == Wake::stop);
    }
    {
        StopWakeup stop_channel;
        assert(wait_for_bridge_change(-1, stop_channel.descriptor(), interrupted) == Wake::interrupted);
        assert(!stopping.load());
        refused([&] { wait_for_bridge_change(-1, stop_channel.descriptor(), impossible_timeout); });
        refused([] { wait_for_bridge_change(-1, 999999); });
    }
    {
        StopWakeup stop_channel;
        int sockets[2];
        assert(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets) == 0);
        const char request = 'r';
        assert(::send(sockets[0], &request, 1, MSG_OOB) == 1);
        assert(wait_for_bridge_change(sockets[1], stop_channel.descriptor()) == Wake::request);
        char received{};
        assert(::recv(sockets[1], &received, 1, MSG_OOB) == 1 && received == request);
        ::close(sockets[0]);
        refused([&] { wait_for_bridge_change(sockets[1], stop_channel.descriptor()); });
        ::close(sockets[1]);
    }
    ::alarm(0);
    return 0;
}
