// SPDX-License-Identifier: MIT
#include "record.hpp"
#include <systemd/sd-json.h>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <memory>
#include <set>
#include <stdexcept>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace uke::battery::record {
[[noreturn]] void reject(const char *reason) { throw std::runtime_error(reason); }
V *field(V *v, const char *key) {
    if (!sd_json_variant_is_object(v)) reject("profile_object_required");
    V *value = sd_json_variant_by_key(v, key);
    if (!value) reject("profile_field_missing");
    return value;
}
std::string string(V *v) {
    if (!sd_json_variant_is_string(v)) reject("profile_string_required");
    std::string value = sd_json_variant_string(v);
    if (value.empty() || value.size() > 128 || value.find_first_not_of(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._:-") != std::string::npos)
        reject("profile_identifier_invalid");
    return value;
}
std::string digest(V *v) {
    const auto value = string(v);
    if (value.size() != 64 || value.find_first_not_of("0123456789abcdef") != std::string::npos)
        reject("profile_sha256_required");
    return value;
}
bool boolean(V *v) {
    if (!sd_json_variant_is_boolean(v)) reject("profile_boolean_required");
    return sd_json_variant_boolean(v);
}
uint64_t integer(V *v) {
    if (!sd_json_variant_is_unsigned(v)) reject("profile_unsigned_integer_required");
    return sd_json_variant_unsigned(v);
}
Number quantity(V *v, bool allow_zero) {
    if (sd_json_variant_is_null(v)) return {};
    if (!sd_json_variant_is_number(v)) reject("profile_number_required");
    const double n = sd_json_variant_real(v);
    if (!std::isfinite(n) || n < 0 || (!allow_zero && n == 0) || n > 9007199254740991.0)
        reject("profile_quantity_invalid");
    return n;
}
void unique(V *v, unsigned depth = 0) {
    if (depth > 12 || sd_json_variant_elements(v) > 128) reject("profile_structure_limit");
    if (sd_json_variant_is_object(v)) {
        std::set<std::string> names;
        for (size_t i = 0; i < sd_json_variant_elements(v); i += 2) {
            V *key = sd_json_variant_by_index(v, i);
            if (!sd_json_variant_is_string(key) || !names.insert(sd_json_variant_string(key)).second)
                reject("profile_duplicate_key");
            unique(sd_json_variant_by_index(v, i + 1), depth + 1);
        }
    } else if (sd_json_variant_is_array(v)) {
        for (size_t i = 0; i < sd_json_variant_elements(v); ++i)
            unique(sd_json_variant_by_index(v, i), depth + 1);
    }
}
void keys(V *v, std::initializer_list<const char *> allowed) {
    if (!sd_json_variant_is_object(v)) reject("profile_object_required");
    const std::set<std::string> names(allowed.begin(), allowed.end());
    for (size_t i = 0; i < sd_json_variant_elements(v); i += 2)
        if (!names.count(sd_json_variant_string(sd_json_variant_by_index(v, i))))
            reject("profile_unknown_key");
}
Json read_json(const std::string &path, uid_t trusted_uid, std::string *bytes) {
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) reject(errno == ENOENT ? "qualification_record_absent" : "qualification_record_unreadable");
    struct stat st{};
    if (fstat(fd, &st) || !S_ISREG(st.st_mode) || st.st_uid != trusted_uid ||
        (st.st_mode & 0022) || st.st_size < 2 || st.st_size > 32768) {
        close(fd); reject("qualification_record_untrusted");
    }
    std::string data; char buffer[4096];
    for (;;) {
        const auto n = read(fd, buffer, sizeof(buffer));
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 || (n > 0 && data.size() + size_t(n) > 32768)) {
            close(fd); reject("qualification_record_read_failed");
        }
        if (!n) break;
        data.append(buffer, size_t(n));
    }
    close(fd);
    if (data.find('\0') != std::string::npos) reject("profile_embedded_nul");
    V *value = nullptr;
    if (sd_json_parse(data.c_str(), sd_json_parse_flags_t(0), &value, nullptr, nullptr) < 0)
        reject("profile_json_invalid");
    Json result(value, &sd_json_variant_unref); unique(value);
    if (bytes) *bytes = data;
    return result;
}
void write_private_json(const std::string &directory, const std::string &name,
                        const std::string &bytes, uid_t trusted_uid) {
    if (name.empty() || name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-.") != name.npos ||
        name == "." || name == ".." || bytes.size() < 2 || bytes.size() > 32768 ||
        bytes.find('\0') != bytes.npos) reject("identity_output_invalid");
    const int dir = open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    struct stat st{};
    if (dir < 0) reject("identity_directory_unavailable");
    if (fstat(dir, &st) || st.st_uid != trusted_uid || (st.st_mode & 0022)) {
        close(dir); reject("identity_directory_untrusted");
    }
    const std::string temporary = "." + name + "-" + std::to_string(getpid());
    int fd = openat(dir, temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (fd < 0) { close(dir); reject("identity_create_failed"); }
    try {
        size_t done = 0;
        while (done < bytes.size()) {
            const auto n = write(fd, bytes.data() + done, bytes.size() - done);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) reject("identity_write_failed");
            done += size_t(n);
        }
        if (fsync(fd)) reject("identity_sync_failed");
        const int closed = close(fd); fd = -1;
        if (closed) reject("identity_close_failed");
        // Rename replaces the directory entry itself, never a symlink target.
        if (renameat(dir, temporary.c_str(), dir, name.c_str()) || fsync(dir))
            reject("identity_commit_failed");
        close(dir);
    } catch (...) {
        if (fd >= 0) close(fd);
        unlinkat(dir, temporary.c_str(), 0); close(dir); throw;
    }
}
}
