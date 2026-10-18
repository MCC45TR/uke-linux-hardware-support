// SPDX-License-Identifier: MIT
// Synthetic host records only. No firmware identities or numerical Uke defaults.
#include "../src/battery/control.hpp"
#include <openssl/evp.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
using namespace uke::battery::control;
namespace {
std::string replace(std::string text, const std::string &from, const std::string &to) {
    size_t at = 0; while ((at = text.find(from, at)) != std::string::npos) { text.replace(at, from.size(), to); at += to.size(); }
    return text;
}
std::string hash(const std::string &text) {
    unsigned char sum[EVP_MAX_MD_SIZE]; unsigned n = 0;
    assert(EVP_Digest(text.data(), text.size(), sum, &n, EVP_sha256(), nullptr) == 1 && n == 32);
    const char hex[] = "0123456789abcdef"; std::string out;
    for (unsigned i = 0; i < n; ++i) { out += hex[sum[i] >> 4]; out += hex[sum[i] & 15]; }
    return out;
}
void save(const std::string &path, const std::string &data) {
    std::ofstream out(path); out << data; out.close(); assert(out.good()); assert(chmod(path.c_str(), 0600) == 0);
}
}
int main(int argc, char **argv) {
    assert(argc == 3);
    const std::string path = std::string(argv[2]) + "/profile.json", identity = std::string(argv[2]) + "/identity.json";
    Context c; c.provider_verified = true; c.kernel_release = "synthetic-test"; c.boot_id = "synthetic-boot";
    c.boottime_ns = 2000000000ULL; c.state.compiled = true; c.state.current_compiled = true;
    c.state.instance = 42; c.state.firmware = 2; c.state.source = 3; c.state.invalidation_ns = 500000000;
    assert(!resolve(c, argv[1], identity, getuid()).qualified);
    const std::string sha(64, 'a');
    const auto profile = replace(R"({
"schema_version":1,"kind":"control_qualification","device":"uke","qualification":"qualified","enable_control":true,
"backend":"qti_uke_charge_control_v1","evidence_id":"@H@",
"identities":{"kernel_release":"synthetic-test","firmware_sha256":"@H@","pack_identity_sha256":"@H@","dt_sha256":"@H@","module_manifest_sha256":"@H@"},
"proofs":{"firmware_abi":"@H@","ack_envelope":"@H@","threshold_readback":"@H@","enable_feature":"@H@","autonomous_protection":"@H@","measurements":"@H@","source_lifecycle":"@H@","current_abi":"@H@","current_readback":"@H@","firmware_cv":"@H@","current_restore":"@H@"},
"limits":{"temperature_min_deciC":100,"temperature_max_deciC":400,"battery_voltage_min_uv":3000000,"battery_voltage_max_uv":4400000,"battery_current_abs_max_ua":3000000,"lease_ns":30000000000,"ack_bytes":12},
"source_type":1,"cap_resume_percent":70,"may_change_feature":true,"standard_clients":true,"current_control":true,
"current_limits":{"minimum_ua":100000,"maximum_ua":2000000,"slow_ua":500000,"restore_ua":1000000,"source_budget_battery_ua":1500000,"increase_step_ua":100000,"increase_dwell_ns":10000000000,"measurement_max_age_ns":5000000000,
"thermal_bands":[{"minimum_deciC":100,"maximum_deciC":400,"maximum_ua":2000000,"maximum_uv":4400000}]}})", "@H@", sha);
    const auto attestation = replace(R"({
"schema_version":1,"kind":"control_identity_attestation","provider":"pmic_glink_power_supply","boot_id":"synthetic-boot",
"driver_instance_boottime_ns":42,"firmware_generation":2,"source_event_generation":3,"reviewed_at_boottime_ns":1000000000,"expires_boottime_ns":60000000000,
"reviewed_control":true,"evidence_id":"@H@","control_profile_sha256":"@P@",
"identities":{"kernel_release":"synthetic-test","firmware_sha256":"@H@","pack_identity_sha256":"@H@","dt_sha256":"@H@","module_manifest_sha256":"@H@"}})", "@H@", sha);
    save(path, profile); save(identity, replace(attestation, "@P@", hash(profile)));
    auto result = resolve(c, path, identity, getuid());
    assert(result.qualified && result.current_control && result.maximum_ua == 2000000 && result.evidence == hash(profile));
    auto changed = c; ++changed.state.source; assert(!resolve(changed, path, identity, getuid()).qualified);
    changed = c; changed.state.invalidation_ns = 1000000000; assert(!resolve(changed, path, identity, getuid()).qualified);
    changed = c; changed.boottime_ns = 60000000000; assert(!resolve(changed, path, identity, getuid()).qualified);
    changed = c; changed.state.current_compiled = false; assert(!resolve(changed, path, identity, getuid()).qualified);
    save(path, profile + " "); assert(!resolve(c, path, identity, getuid()).qualified); // Exact bytes bound.
    auto malformed = replace(profile, "\"source_type\":1", "\"source_type\":1,\"source_type\":1");
    save(path, malformed); assert(!resolve(c, path, identity, getuid()).qualified);
    malformed = replace(profile, "\"source_type\":1", "\"source_type\":1,\"unknown\":true");
    save(path, malformed); assert(!resolve(c, path, identity, getuid()).qualified);
    malformed = replace(profile, "\"minimum_ua\":100000", "\"minimum_ua\":-1");
    save(path, malformed); assert(!resolve(c, path, identity, getuid()).qualified);
    malformed = replace(profile, "\"maximum_ua\":2000000", "\"maximum_ua\":4294967296");
    save(path, malformed); assert(!resolve(c, path, identity, getuid()).qualified);
    malformed = replace(profile, "\"minimum_deciC\":100", "\"minimum_deciC\":400");
    save(path, malformed); assert(!resolve(c, path, identity, getuid()).qualified);
    save(path, profile); assert(chmod(path.c_str(), 0666) == 0); assert(!resolve(c, path, identity, getuid()).qualified);
    assert(chmod(path.c_str(), 0600) == 0);
    const std::string link = std::string(argv[2]) + "/link.json";
    std::filesystem::create_symlink(path, link); assert(!resolve(c, link, identity, getuid()).qualified);
}
