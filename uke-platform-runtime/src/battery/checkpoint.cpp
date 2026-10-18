// SPDX-License-Identifier: MIT
#include "model.hpp"
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>

namespace uke::battery {
namespace {
uint64_t checksum(const std::string &s) {
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
    return h;
}
bool token(const std::string &s) {
    return !s.empty() && s.size() <= 128 &&
        s.find_first_not_of("0123456789abcdefABCDEF:-_") == std::string::npos;
}
bool sane(const EfcState &s) {
    return token(s.battery_id) && token(s.boot_id) && token(s.epoch) &&
        std::isfinite(s.reference_uah) && s.reference_uah > 0 &&
        std::isfinite(s.discharge_uah) && s.discharge_uah >= 0 &&
        std::isfinite(s.covered_seconds) && s.covered_seconds >= 0 &&
        std::isfinite(s.uncovered_seconds) && s.uncovered_seconds >= 0 &&
        (!s.last_current_ua || std::isfinite(*s.last_current_ua));
}
int directory_fd(const std::string &path) {
    const int fd = open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) throw std::system_error(errno, std::generic_category(), "checkpoint directory");
    struct stat st{};
    if (fstat(fd, &st) || st.st_uid != geteuid() || (st.st_mode & 0077)) {
        close(fd); throw std::runtime_error("checkpoint directory must be private and owned by this user");
    }
    return fd;
}
}
static std::optional<EfcState> read_checkpoint(const std::string &directory, const char *name);
void save_checkpoint(const std::string &directory, const EfcState &s) {
    if (!sane(s)) throw std::runtime_error("invalid checkpoint");
    std::ostringstream body;
    body << std::setprecision(17) << "1 " << s.sequence << ' ' << s.battery_id << ' ' << s.boot_id << ' '
         << s.epoch << ' ' << s.reference_uah << ' ' << s.discharge_uah << ' ' << s.covered_seconds << ' '
         << s.uncovered_seconds << ' ' << s.last_boottime_ns << ' ' << s.last_monotonic_ns << ' '
         << bool(s.last_current_ua) << ' ' << s.last_current_ua.value_or(0) << '\n';
    const std::string payload = body.str();
    const std::string record = payload + std::to_string(checksum(payload)) + '\n';
    const int dir = directory_fd(directory);
    const int lock = openat(dir, ".efc.lock", O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (lock < 0 || flock(lock, LOCK_EX | LOCK_NB)) {
        const int error = errno; if (lock >= 0) close(lock); close(dir);
        throw std::system_error(error, std::generic_category(), "checkpoint writer lock");
    }
    const std::string temporary = ".efc-" + std::to_string(getpid()) + "-" + std::to_string(s.sequence);
    int fd = openat(dir, temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    try {
        if (fd < 0) throw std::system_error(errno, std::generic_category(), "checkpoint create");
        const auto newest = load_checkpoint(directory);
        const auto previous = read_checkpoint(directory, "efc.state");
        if (newest && newest->sequence >= s.sequence)
            throw std::runtime_error("non-increasing checkpoint sequence refused");
        size_t done = 0;
        while (done < record.size()) {
            const ssize_t n = write(fd, record.data() + done, record.size() - done);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) throw std::system_error(n < 0 ? errno : EIO, std::generic_category(), "checkpoint write");
            done += static_cast<size_t>(n);
        }
        if (fsync(fd)) throw std::system_error(errno, std::generic_category(), "checkpoint sync");
        if (close(fd)) { fd = -1; throw std::system_error(errno, std::generic_category(), "checkpoint close"); }
        fd = -1;
        if (previous) {
            unlinkat(dir, ".efc.previous.new", 0);
            if (linkat(dir, "efc.state", dir, ".efc.previous.new", 0) ||
                renameat(dir, ".efc.previous.new", dir, "efc.previous") || fsync(dir))
                throw std::system_error(errno, std::generic_category(), "checkpoint previous record");
        }
        if (renameat(dir, temporary.c_str(), dir, "efc.state") || fsync(dir))
            throw std::system_error(errno, std::generic_category(), "checkpoint commit");
        close(lock); close(dir);
    } catch (...) {
        if (fd >= 0) close(fd);
        unlinkat(dir, temporary.c_str(), 0); unlinkat(dir, ".efc.previous.new", 0); close(lock); close(dir); throw;
    }
}
static std::optional<EfcState> read_checkpoint(const std::string &directory, const char *name) {
    int dir = -1, fd = -1;
    try {
        dir = directory_fd(directory);
        fd = openat(dir, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        close(dir); dir = -1;
        if (fd < 0) return {};
        struct stat st{};
        if (fstat(fd, &st) || !S_ISREG(st.st_mode) || st.st_size <= 0 || st.st_size > 2048 ||
            st.st_uid != geteuid() || (st.st_mode & 0077)) { close(fd); return {}; }
        std::string record; char data[2049];
        for (;;) {
            const ssize_t n = read(fd, data, sizeof(data));
            if (n < 0 && errno == EINTR) continue;
            if (n < 0 || record.size() + static_cast<size_t>(n) > 2048) { close(fd); return {}; }
            if (!n) break;
            record.append(data, static_cast<size_t>(n));
        }
        close(fd); fd = -1;
        const size_t line = record.find('\n');
        if (line == std::string::npos) return {};
        const std::string payload = record.substr(0, line + 1);
        std::istringstream hash(record.substr(line + 1)); uint64_t expected;
        if (!(hash >> expected) || checksum(payload) != expected || !(hash >> std::ws).eof()) return {};
        EfcState s; int version, present; double current;
        std::istringstream input(payload);
        if (!(input >> version >> s.sequence >> s.battery_id >> s.boot_id >> s.epoch >> s.reference_uah
              >> s.discharge_uah >> s.covered_seconds >> s.uncovered_seconds >> s.last_boottime_ns
              >> s.last_monotonic_ns >> present >> current) || version != 1 ||
            (present != 0 && present != 1) || !(input >> std::ws).eof()) return {};
        if (present) s.last_current_ua = current;
        if (!sane(s)) return {};
        return s;
    } catch (...) { if (fd >= 0) close(fd); if (dir >= 0) close(dir); return {}; }
}
std::optional<EfcState> load_checkpoint(const std::string &directory) {
    auto current = read_checkpoint(directory, "efc.state");
    return current ? current : read_checkpoint(directory, "efc.previous");
}

}
