// SPDX-License-Identifier: MIT
#pragma once
#include "model.hpp"
#include <sys/types.h>

namespace uke::battery {
struct IdentityContext {
    bool provider_verified = false, fixture = false;
    std::string kernel_release, boot_id;
    uint64_t driver_instance_ns = 0, boottime_ns = 0;
    uint32_t firmware_generation = 0;
};
struct ResolvedProfile {
    Qualification qualification;
    std::string battery_id;
    std::string reason = "identity_unqualified";
    std::string assurance = "none";
};
// This record qualifies observation models only; no result authorizes a SET.
ResolvedProfile resolve_profile(const IdentityContext &context,
    const std::string &profile = "/usr/share/uke-battery/model-profile.json",
    const std::string &identity = "/run/uke-battery/identity.json", uid_t trusted_uid = 0);
}
