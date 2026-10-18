// SPDX-License-Identifier: MIT
// Read-only evdev contact monitor; no grab or synthetic input.
#include <linux/input.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <charconv>
#include <string_view>

int contact_test(int argc, char **argv) {
    if (argc != 3) return 64;
    int seconds{};std::string_view arg=argv[2];auto parsed=std::from_chars(arg.data(),arg.data()+arg.size(),seconds);
    if(parsed.ec!=std::errc{}||parsed.ptr!=arg.data()+arg.size()||seconds<1||seconds>300)return 64;
    const int fd = open(argv[1], O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    struct Close { int fd; ~Close(){if(fd>=0)close(fd);} } closer{fd};
    if (fd < 0) { std::perror("open"); return 1; }
    std::array<unsigned long, (KEY_MAX + 8 * sizeof(unsigned long)) / (8 * sizeof(unsigned long))> keys{};
    std::array<int, 11> slots{};
    slots[0] = ABS_MT_TRACKING_ID;
    if (ioctl(fd, EVIOCGKEY(sizeof(keys)), keys.data()) < 0 ||
        ioctl(fd, EVIOCGMTSLOTS(sizeof(slots)), slots.data()) < 0) {
        std::perror("initial input state"); return 1;
    }
    constexpr auto bits = 8 * sizeof(unsigned long);
    const bool pressed = (keys[BTN_TOUCH / bits] >> (BTN_TOUCH % bits)) & 1;
    std::printf("initial BTN_TOUCH=%d tracking_ids=", pressed);
    for (std::size_t i = 1; i < slots.size(); ++i) std::printf("%d,", slots[i]);
    std::puts(""); std::fflush(stdout);
    if (!pressed) return 3;
    const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    pollfd pfd{fd, POLLIN, 0};
    while (std::chrono::steady_clock::now() < end) {
        const int ready = poll(&pfd, 1, 100);
        if (ready < 0 || (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))) return 1;
        if (!ready) continue;
        std::array<input_event, 128> events{};
        const auto bytes = read(fd, events.data(), sizeof(events));
        if (bytes <= 0 || bytes % sizeof(input_event)) return 1;
        for (std::size_t i = 0; i < static_cast<std::size_t>(bytes) / sizeof(input_event); ++i) {
            const auto &e = events[i];
            if ((e.type == EV_KEY && e.code == BTN_TOUCH) ||
                (e.type == EV_ABS && e.code == ABS_MT_TRACKING_ID) ||
                (e.type == EV_SYN && e.code == SYN_DROPPED)) {
                std::printf("%lld.%06lld type=%u code=%u value=%d\n",
                    static_cast<long long>(e.time.tv_sec), static_cast<long long>(e.time.tv_usec),
                    e.type, e.code, e.value);
                std::fflush(stdout);
                if ((e.type == EV_KEY && !e.value) ||
                    (e.type == EV_ABS && e.value < 0) || e.type == EV_SYN) return 2;
            }
        }
    }
    std::puts("PASS: original contact retained through observation window");
    return 0;
}
