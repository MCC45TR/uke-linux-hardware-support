// SPDX-License-Identifier: MIT
#include "../src/battery/profile.hpp"
#include <cassert>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
using namespace uke::battery;
static void put(const std::string &path, const std::string &data) {
    std::ofstream out(path, std::ios::trunc); out << data; out.close(); assert(out);
    assert(chmod(path.c_str(), 0600) == 0);
}
static std::string replace(std::string value, const std::string &old, const std::string &next) {
    auto pos = value.find(old); assert(pos != std::string::npos); value.replace(pos, old.size(), next); return value;
}
int main(int argc, char **argv) {
    assert(argc == 2);
    const std::string profile = std::string(argv[1]) + "/profile.json", identity = std::string(argv[1]) + "/identity.json";
    const std::string hash(64, 'a');
    const std::string ids = "{\"kernel_release\":\"fixture-1\",\"firmware_sha256\":\"" + hash +
        "\",\"pack_identity_sha256\":\"" + hash + "\",\"dt_sha256\":\"" + hash + "\",\"module_manifest_sha256\":\"" + hash + "\"}";
    const std::string p = "{\"schema_version\":1,\"kind\":\"model_qualification\",\"device\":\"uke\","
        "\"profile_id\":\"fixture-profile\",\"qualification\":\"qualified\",\"enable_control\":false,\"identities\":" + ids +
        ",\"measurements\":{\"current\":true,\"energy\":true,\"learned_capacity\":true,\"capacity_basis\":true,\"target_effective\":false},"
        "\"efc_reference_uah\":1000000,\"reserve_uwh\":100000,\"evidence_id\":\"" + hash + "\"}";
    const std::string a = "{\"schema_version\":1,\"kind\":\"running_identity_attestation\",\"provider\":\"pmic_glink_power_supply\","
        "\"boot_id\":\"b1\",\"driver_instance_boottime_ns\":1,\"firmware_generation\":2,\"reviewed_at_boottime_ns\":3,"
        "\"expires_boottime_ns\":100000000000,\"identities\":" + ids + ",\"reviewed_identity\":true,\"evidence_id\":\"" + hash + "\"}";
    IdentityContext c; c.provider_verified = true; c.boot_id = "b1"; c.kernel_release = "fixture-1";
    c.driver_instance_ns = 1; c.firmware_generation = 2; c.boottime_ns = 4;
    auto resolve = [&] { return resolve_profile(c, profile, identity, geteuid()); };
    assert(!resolve().qualification.current);
    put(profile, p); put(identity, a);
    auto good = resolve(); assert(good.qualification.current && good.qualification.energy);
    assert(good.battery_id == hash && good.qualification.efc_reference_uah == 1000000);
    assert(good.assurance == "operator_attested_this_boot_not_hardware_authentication");
    for (const auto &bad : {
        replace(p, "\"schema_version\":1", "\"schema_version\":2"),
        replace(p, "\"schema_version\":1", "\"schema_version\":1,\"schema_version\":1"),
        replace(p, "\"enable_control\":false", "\"enable_control\":true"),
        replace(p, "\"current\":true", "\"current\":\"true\""),
        replace(p, "\"current\":true", "\"current\":false"),
        replace(p, "\"efc_reference_uah\":1000000", "\"efc_reference_uah\":0"),
        replace(p, "\"reserve_uwh\":100000", "\"reserve_uwh\":null"),
        replace(p, "\"target_effective\":false", "\"target_effective\":true"),
        replace(p, "\"device\":\"uke\"", "\"device\":\"nabu\""),
        replace(p, "\"enable_control\":false", "\"enable_control\":false,\"raw_property\":12"),
        p + " garbage", std::string(32769, 'x')}) {
        put(profile, bad); auto result = resolve();
        assert(!result.qualification.current && result.battery_id.empty());
    }
    put(profile, p);
    for (const auto &bad : {
        replace(a, "\"boot_id\":\"b1\"", "\"boot_id\":\"b2\""),
        replace(a, "\"firmware_generation\":2", "\"firmware_generation\":3"),
        replace(a, "\"driver_instance_boottime_ns\":1", "\"driver_instance_boottime_ns\":9"),
        replace(a, "\"expires_boottime_ns\":100000000000", "\"expires_boottime_ns\":4"),
        replace(a, "\"expires_boottime_ns\":100000000000", "\"expires_boottime_ns\":900000000004"),
        replace(a, "\"reviewed_identity\":true", "\"reviewed_identity\":false"),
        replace(a, "\"dt_sha256\":\"" + hash, "\"dt_sha256\":\"" + std::string(64, 'b'))}) {
        put(identity, bad); assert(!resolve().qualification.current);
    }
    put(identity, a); assert(chmod(identity.c_str(), 0620) == 0); assert(!resolve().qualification.current);
    put(identity, a); c.fixture = true; assert(!resolve().qualification.current); c.fixture = false;
    c.provider_verified = false; assert(!resolve().qualification.current); c.provider_verified = true;
    assert(unlink(profile.c_str()) == 0); assert(symlink(identity.c_str(), profile.c_str()) == 0);
    assert(!resolve().qualification.current);
    return 0;
}
