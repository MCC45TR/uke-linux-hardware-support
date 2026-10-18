// SPDX-License-Identifier: MIT
#pragma once
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace uke::color {
inline void file_need(bool ok) { if (!ok) throw std::runtime_error("Front-color snapshot publication failed"); }
inline void check_target(int directory) {
    struct stat st{};
    if (fstatat(directory, "front-color.json", &st, AT_SYMLINK_NOFOLLOW) == 0)
        file_need(S_ISREG(st.st_mode) && st.st_uid == geteuid() && st.st_nlink == 1 && !(st.st_mode & 0022));
    else file_need(errno == ENOENT);
}
inline void remove_snapshot(int directory) {
    check_target(directory);
    file_need(unlinkat(directory, "front-color.json", 0) == 0 || errno == ENOENT);
}
inline void write_snapshot(int directory, const std::string &json) {
    file_need(!json.empty() && json.size() <= 4096);
    check_target(directory);
    static std::uint64_t sequence = 0;
    const auto temporary = ".front-color-" + std::to_string(getpid()) + "-" + std::to_string(++sequence);
    int fd = openat(directory, temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    file_need(fd >= 0);
    try {
        std::size_t done = 0;
        while (done < json.size()) {
            auto n = write(fd, json.data() + done, json.size() - done);
            if (n < 0 && errno == EINTR) continue;
            file_need(n > 0); done += std::size_t(n);
        }
        file_need(fchmod(fd, 0644) == 0 && fsync(fd) == 0);
        const auto closed = close(fd); fd = -1; file_need(closed == 0);
        check_target(directory);
        file_need(renameat(directory, temporary.c_str(), directory, "front-color.json") == 0);
        file_need(fsync(directory) == 0);
    } catch (...) {
        if (fd >= 0) close(fd);
        unlinkat(directory, temporary.c_str(), 0);
        throw;
    }
}
} // namespace uke::color
