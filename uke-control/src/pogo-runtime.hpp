// SPDX-License-Identifier: MIT
#pragma once
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <initializer_list>
#include <poll.h>
#include <stdexcept>
#include <string_view>
#include <sys/eventfd.h>
#include <unistd.h>

namespace uke_pogo_runtime {

// These cumulative families retain the version-1 kernel authentication bridge.
// Board, driver, TA, challenge and token admission remain separate checks.
inline bool published_unstable_release(std::string_view release)
{
    constexpr std::string_view suffix = "-unstable";
    if (!release.ends_with(suffix))
        return false;
    release.remove_suffix(suffix.size());
    bool family = false;
    for (const auto prefix : {std::string_view{"7.3.1-"}, std::string_view{"7.3.3-"}}) {
        if (release.starts_with(prefix)) {
            release.remove_prefix(prefix.size());
            family = true;
            break;
        }
    }
    if (!family || release.empty() || release.front() == '0')
        return false;
    for (const char byte : release) {
        if (byte < '0' || byte > '9')
            return false;
    }
    return true;
}

inline std::atomic<bool> stopping{false};
inline std::atomic<int> stop_descriptor{-1};
static_assert(std::atomic<bool>::is_always_lock_free);
static_assert(std::atomic<int>::is_always_lock_free);

inline void stop(int) noexcept
{
    const int saved_errno = errno;
    stopping.store(true, std::memory_order_relaxed);
    const int descriptor = stop_descriptor.load(std::memory_order_relaxed);
    if (descriptor >= 0) {
        const uint64_t one = 1;
        ssize_t result;
        do {
            result = ::write(descriptor, &one, sizeof(one));
        } while (result < 0 && errno == EINTR);
        // EAGAIN means that an earlier stop notification is already pending.
    }
    errno = saved_errno;
}

class StopWakeup {
public:
    StopWakeup()
    {
        if (stop_descriptor.load(std::memory_order_relaxed) != -1)
            throw std::runtime_error("A pogo stop channel is already installed");
        descriptor_ = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (descriptor_ < 0)
            throw std::runtime_error("Cannot create the pogo stop notification");
        stopping.store(false, std::memory_order_relaxed);
        stop_descriptor.store(descriptor_, std::memory_order_relaxed);
        struct sigaction action{};
        action.sa_handler = stop;
        sigemptyset(&action.sa_mask);
        sigaddset(&action.sa_mask, SIGTERM);
        sigaddset(&action.sa_mask, SIGINT);
        if (::sigaction(SIGTERM, &action, &old_term_) < 0) {
            close_failed();
            throw std::runtime_error("Cannot install the pogo stop handler");
        }
        if (::sigaction(SIGINT, &action, &old_int_) < 0) {
            ::sigaction(SIGTERM, &old_term_, nullptr);
            close_failed();
            throw std::runtime_error("Cannot install the pogo interrupt handler");
        }
    }

    ~StopWakeup()
    {
        // The owner constructs this before its TEE backend, so backend teardown
        // finishes before the signal channel is retired.
        stop_descriptor.store(-1, std::memory_order_relaxed);
        ::sigaction(SIGINT, &old_int_, nullptr);
        ::sigaction(SIGTERM, &old_term_, nullptr);
        ::close(descriptor_);
    }

    StopWakeup(const StopWakeup&) = delete;
    StopWakeup& operator=(const StopWakeup&) = delete;
    int descriptor() const noexcept { return descriptor_; }

private:
    void close_failed() noexcept
    {
        stop_descriptor.store(-1, std::memory_order_relaxed);
        ::close(descriptor_);
        descriptor_ = -1;
    }
    int descriptor_ = -1;
    struct sigaction old_term_{}, old_int_{};
};

enum class Wake { request, stop, interrupted };
using Poll = int (*)(struct pollfd*, nfds_t, int);

inline Wake wait_for_bridge_change(int request, int stop_fd, Poll poll_call = ::poll)
{
    struct pollfd events[]{{request, POLLPRI | POLLERR, 0}, {stop_fd, POLLIN, 0}};
    const int status = poll_call(events, 2, -1);
    if (status < 0 && errno == EINTR)
        return stopping.load(std::memory_order_relaxed) ? Wake::stop : Wake::interrupted;
    if (status <= 0)
        throw std::runtime_error("Cannot wait for a pogo authentication event");
    if (events[1].revents & POLLIN) {
        stopping.store(true, std::memory_order_relaxed);
        return Wake::stop;
    }
    if (events[1].revents & (POLLERR | POLLHUP | POLLNVAL))
        throw std::runtime_error("The pogo stop notification was removed");
    if (events[0].revents & (POLLHUP | POLLNVAL))
        throw std::runtime_error("The pogo authentication bridge was removed");
    if (events[0].revents & POLLPRI)
        return Wake::request;
    throw std::runtime_error("The pogo authentication bridge has no valid event");
}

} // namespace uke_pogo_runtime
