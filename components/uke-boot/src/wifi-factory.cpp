// SPDX-License-Identifier: MIT
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <unistd.h>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void require(bool ok, const char* reason) {
    if (!ok) throw std::runtime_error(reason);
}

std::string factory_mac(std::string_view text) {
    require(text.size() <= 1024, "Factory WLAN record is too large");
    std::string value;
    bool seen = false;
    while (!text.empty()) {
        const auto end = text.find('\n');
        auto line = text.substr(0, end);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (line.substr(0, 6) == "wlan0=") {
            require(!seen, "Duplicate factory WLAN address");
            seen = true;
            value = std::string(line.substr(6));
        }
        if (end == text.npos) break;
        text.remove_prefix(end + 1);
    }
    require(value.size() == 12, "Missing or invalid factory WLAN address");
    std::array<unsigned char, 6> bytes{};
    bool nonzero = false;
    for (size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];
        const int digit = c >= '0' && c <= '9' ? c - '0' :
            c >= 'a' && c <= 'f' ? c - 'a' + 10 :
            c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
        require(digit >= 0, "Invalid factory WLAN hexadecimal digit");
        bytes[i / 2] = bytes[i / 2] * 16 + static_cast<unsigned>(digit);
        nonzero |= digit != 0;
    }
    require(nonzero && !(bytes[0] & 1), "Factory WLAN address is not unicast");
    std::array<char, 18> address{};
    std::snprintf(address.data(), address.size(), "%02X:%02X:%02X:%02X:%02X:%02X",
                  unsigned(bytes[0]), unsigned(bytes[1]), unsigned(bytes[2]),
                  unsigned(bytes[3]), unsigned(bytes[4]), unsigned(bytes[5]));
    return address.data();
}

std::string factory_record(int root, dev_t expected_device) {
    int directory = -1, input = -1;
    try {
        directory = openat(root, "wlan", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        require(directory >= 0, "Factory WLAN directory is unavailable");
        input = openat(directory, "wlan_mac.bin", O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
        require(input >= 0, "Factory WLAN record is unavailable");
        struct stat parent{}, record{};
        require(fstat(directory, &parent) == 0 && fstat(input, &record) == 0 &&
                parent.st_dev == expected_device && record.st_dev == expected_device &&
                S_ISREG(record.st_mode), "Factory WLAN record is not a regular file on persist");
        require(record.st_size > 0 && record.st_size <= 1024, "Invalid factory WLAN record size");
        std::array<char, 1025> data{};
        size_t size = 0;
        while (size < data.size()) {
            const auto count = read(input, data.data() + size, data.size() - size);
            if (count < 0 && errno == EINTR) continue;
            require(count >= 0, "Factory WLAN record read failed");
            if (!count) break;
            size += static_cast<size_t>(count);
        }
        require(size == static_cast<size_t>(record.st_size), "Factory WLAN record size changed");
        close(input); input = -1;
        close(directory); directory = -1;
        return std::string(data.data(), size);
    } catch (...) {
        if (input >= 0) close(input);
        if (directory >= 0) close(directory);
        throw;
    }
}

void provision() {
    constexpr auto persist = "/mnt/vendor/persist";
    struct stat mount{}, partition{};
    struct statvfs flags{};
    const int root = open(persist, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    require(root >= 0, "Factory persist directory is unavailable");
    std::string record;
    try {
        require(fstat(root, &mount) == 0 &&
                stat("/dev/disk/by-partlabel/persist", &partition) == 0 &&
                S_ISBLK(partition.st_mode) && mount.st_dev == partition.st_rdev,
                "Factory persist is not the physical persist partition");
        require(fstatvfs(root, &flags) == 0 &&
                (flags.f_flag & (ST_RDONLY | ST_NODEV | ST_NOSUID | ST_NOEXEC)) ==
                (ST_RDONLY | ST_NODEV | ST_NOSUID | ST_NOEXEC),
                "Factory persist must be mounted read-only, nodev, nosuid and noexec");
        record = factory_record(root, partition.st_rdev);
    } catch (...) { close(root); throw; }
    close(root);
    const auto mac = factory_mac(record);
    const auto config = "[connection-uke-factory-wifi]\nmatch-device=driver:ath11k\n"
                        "wifi.cloned-mac-address=" + mac + "\n";
    constexpr auto directory = "/run/NetworkManager/conf.d";
    char temporary[] = "/run/NetworkManager/conf.d/.uke-factory-wifi.XXXXXX";
    constexpr auto destination = "/run/NetworkManager/conf.d/60-uke-factory-wifi.conf";
    const int output = mkostemp(temporary, O_CLOEXEC);
    require(output >= 0, "Cannot create private NetworkManager configuration");
    const auto written = write(output, config.data(), config.size());
    const bool saved = written == static_cast<ssize_t>(config.size()) && fsync(output) == 0;
    const bool closed = close(output) == 0;
    if (!saved || !closed || rename(temporary, destination) != 0) {
        unlink(temporary);
        throw std::runtime_error("Cannot publish factory NetworkManager configuration");
    }
    const int dir = open(directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    require(dir >= 0, "Cannot open NetworkManager configuration directory");
    const bool durable = fsync(dir) == 0;
    close(dir);
    require(durable, "Cannot sync NetworkManager configuration directory");
}
}

int main() {
    try {
        provision();
        std::puts("Factory WLAN address admitted from read-only persist");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Factory WLAN provisioning failed: %s\n", error.what());
        return 1;
    }
}
