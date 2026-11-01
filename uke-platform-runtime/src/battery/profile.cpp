// SPDX-License-Identifier: MIT
#include "profile.hpp"
#include <systemd/sd-json.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <set>
#include <stdexcept>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace uke::battery {
namespace {
using Json = std::unique_ptr<sd_json_variant, decltype(&sd_json_variant_unref)>;
using V = sd_json_variant;
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
Json read_json(const std::string &path, uid_t trusted_uid) {
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
    Json result(value, &sd_json_variant_unref); unique(value); return result;
}
}
ResolvedProfile resolve_profile(const IdentityContext &c, const std::string &profile,
                                const std::string &identity, uid_t trusted_uid) {
    ResolvedProfile result;
    if (c.fixture || !c.provider_verified || c.boot_id.empty() || c.kernel_release.empty() || !c.driver_instance_ns || !c.boottime_ns)
        return result;
    try {
        const auto p = read_json(profile, trusted_uid);
        keys(p.get(), {"schema_version", "kind", "device", "profile_id", "qualification", "enable_control",
                       "identities", "measurements", "efc_reference_uah", "reserve_uwh", "evidence_id"});
        if (integer(field(p.get(), "schema_version")) != 1 || string(field(p.get(), "kind")) != "model_qualification" ||
            string(field(p.get(), "device")) != "uke" || boolean(field(p.get(), "enable_control")))
            reject("profile_control_or_schema_rejected");
        const auto qualification = string(field(p.get(), "qualification"));
        if (qualification == "unqualified") { result.reason = "profile_unqualified"; return result; }
        if (qualification != "qualified") reject("profile_qualification_invalid");
        Qualification q; q.profile = string(field(p.get(), "profile_id"));
        const auto evidence = digest(field(p.get(), "evidence_id"));
        V *ids = field(p.get(), "identities");
        keys(ids, {"kernel_release", "firmware_sha256", "pack_identity_sha256", "dt_sha256", "module_manifest_sha256"});
        V *measure = field(p.get(), "measurements");
        keys(measure, {"current", "energy", "learned_capacity", "capacity_basis", "target_effective"});
        q.current = boolean(field(measure, "current")); q.energy = boolean(field(measure, "energy"));
        q.learned_capacity = boolean(field(measure, "learned_capacity")); q.capacity_basis = boolean(field(measure, "capacity_basis"));
        // The current provider has no qualified effective-target readback.
        if (boolean(field(measure, "target_effective"))) reject("effective_target_provider_unavailable");
        q.efc_reference_uah = quantity(field(p.get(), "efc_reference_uah"), false);
        q.reserve_uwh = quantity(field(p.get(), "reserve_uwh"), true);
        if ((q.energy && (!q.current || !q.reserve_uwh)) || (q.learned_capacity && !q.capacity_basis) ||
            (q.efc_reference_uah && (!q.current || !q.capacity_basis))) reject("profile_dependency_missing");
        const auto a = read_json(identity, trusted_uid);
        keys(a.get(), {"schema_version", "kind", "provider", "boot_id", "driver_instance_boottime_ns",
                       "firmware_generation", "reviewed_at_boottime_ns", "expires_boottime_ns",
                       "identities", "reviewed_identity", "evidence_id"});
        if (integer(field(a.get(), "schema_version")) != 1 ||
            string(field(a.get(), "kind")) != "running_identity_attestation" ||
            string(field(a.get(), "provider")) != "pmic_glink_power_supply" ||
            !boolean(field(a.get(), "reviewed_identity")) || digest(field(a.get(), "evidence_id")) != evidence)
            reject("identity_evidence_missing");
        if (string(field(a.get(), "boot_id")) != c.boot_id ||
            integer(field(a.get(), "driver_instance_boottime_ns")) != c.driver_instance_ns ||
            integer(field(a.get(), "firmware_generation")) != c.firmware_generation)
            reject("identity_runtime_mismatch");
        const auto reviewed = integer(field(a.get(), "reviewed_at_boottime_ns"));
        const auto expires = integer(field(a.get(), "expires_boottime_ns"));
        if (reviewed > c.boottime_ns || expires <= c.boottime_ns || expires <= reviewed ||
            expires - reviewed > 900000000000ULL) reject("identity_attestation_expired");
        V *running = field(a.get(), "identities");
        keys(running, {"kernel_release", "firmware_sha256", "pack_identity_sha256", "dt_sha256", "module_manifest_sha256"});
        if (string(field(ids, "kernel_release")) != c.kernel_release ||
            string(field(running, "kernel_release")) != c.kernel_release) reject("identity_kernel_mismatch");
        for (const char *key : {"firmware_sha256", "pack_identity_sha256", "dt_sha256", "module_manifest_sha256"})
            if (digest(field(ids, key)) != digest(field(running, key))) reject("identity_artifact_mismatch");
        result.battery_id = digest(field(running, "pack_identity_sha256"));
        result.qualification = std::move(q);
        result.reason = "model_inputs_qualified";
        result.assurance = "operator_attested_this_boot_not_hardware_authentication";
    } catch (const std::exception &e) { result = {}; result.reason = e.what(); }
    return result;
}
}
