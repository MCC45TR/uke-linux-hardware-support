// SPDX-License-Identifier: MIT
#include "charge-policy.hpp"
#include <charconv>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace uke::charge {
std::string policy_modes_json() {
    return R"({"schema_version":1,"kind":"uke_charge_policy_capabilities","observation":"metadata_only_no_power_supply_get","actuator_client_implemented":true,"qualification_checked":false,"controls_qualified":null,"voltage_control_owner":"firmware","lease_release_restores_settings":false,"modes":[
{"id":"firmware","target":"existing_firmware_policy","rate":"firmware_managed","factory_restore_implemented":false},
{"id":"charge80","end_percent":80,"requires":["qualified_autonomous_end_threshold"]},
{"id":"hold70-80","start_percent":70,"end_percent":80,"requires":["qualified_autonomous_start_threshold","qualified_autonomous_end_threshold"]},
{"id":"hold80-90","start_percent":80,"end_percent":90,"requires":["qualified_autonomous_start_threshold","qualified_autonomous_end_threshold"]},
{"id":"temporary-full","apply_implemented":false,"duration_minutes_min":1,"duration_minutes_max":1440,"return_mode":"hold70-80","requires":["qualified_device_side_expiry_and_return"]},
{"id":"slow","rate":"qualified_pack_slow_current","requires":["qualified_battery_current_control","qualified_firmware_cv","qualified_electrical_profile"]},
{"id":"automatic","rate":"qualified_thermal_current_optimizer","requires":["qualified_battery_current_control","qualified_firmware_cv","qualified_electrical_profile","qualified_source_power_after_load"]}],"health_sources":{"reported_health":"reported_health_label","capacity_retention_percent":"qualified_learned_full_over_design","reported_cycles":"reported_counter_semantics_require_qualification","observed_efc":"observed_discharge_only_with_coverage"},"physical_charge_stopped":null})";
}
int policy_command(int argc, char** argv) {
    if (argc < 3 || std::string_view(argv[1]) != "charge") return -1;
    if (argc == 3 && std::string_view(argv[2]) == "modes") {
        std::cout << policy_modes_json() << '\n'; return 0;
    }
    if ((argc != 4 && argc != 5) || std::string_view(argv[2]) != "plan") return -1;
    const std::string name = argv[3];
    const auto request = preset(name);
    if (!request) throw std::runtime_error("Charge mode must be firmware, charge80, hold70-80, hold80-90, temporary-full, slow or automatic");
    unsigned minutes = 0;
    if (request->target == Target::temporary_full) {
        if (argc != 5) throw std::runtime_error("Temporary full preview requires a duration of 1..1440 minutes");
        const std::string_view text(argv[4]);
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), minutes);
        if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || minutes < 1 || minutes > 1440)
            throw std::runtime_error("Temporary full duration must be 1..1440 minutes");
    } else if (argc != 4) throw std::runtime_error("This charge preview does not accept a duration");
    // Static provider capability is intentionally not inferred from file existence,
    // model attestation, plausible readings or operator-selected preferences.
    Planner planner;
    const auto plan = planner.evaluate(*request, {}, {}, {});
    std::cout << "{\"schema_version\":1,\"kind\":\"uke_charge_policy_plan\",\"requested\":\"" << name
        << "\",\"operation\":\"preview\",\"state\":\"" << plan.state
        << "\",\"reason\":\"" << plan.reason
        << "\",\"qualification_checked\":false,\"effective\":null,\"applied\":false,\"hardware_write\":false,\"persisted\":false";
    if (minutes) std::cout << ",\"duration_minutes\":" << minutes
        << ",\"armed\":false,\"expires_boottime_ns\":null,\"return_mode\":\"hold70-80\",\"apply_implemented\":false";
    std::cout << ",\"physical_charge_stopped\":null}" << '\n';
    return 0;
}
}
