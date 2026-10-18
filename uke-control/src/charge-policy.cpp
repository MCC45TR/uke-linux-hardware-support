// SPDX-License-Identifier: MIT
#include "charge-policy.hpp"
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace uke::charge {
std::string policy_modes_json() {
    return R"({"schema_version":1,"kind":"uke_charge_policy_capabilities","observation":"metadata_only_no_power_supply_get","actuator_implemented":false,"controls_qualified":false,"modes":[{"id":"firmware","target":"firmware_default","rate":"firmware_managed"},{"id":"charge80","end_percent":80,"requires":["qualified_autonomous_end_threshold"]},{"id":"hold80-90","start_percent":80,"end_percent":90,"requires":["qualified_autonomous_start_threshold","qualified_autonomous_end_threshold"]},{"id":"slow","rate":"qualified_pack_slow_current","requires":["qualified_battery_cc_cv","qualified_electrical_profile"]},{"id":"automatic","rate":"qualified_thermal_current_voltage_optimizer","requires":["qualified_battery_cc_cv","qualified_electrical_profile","qualified_source_power_after_load"]}],"health_sources":{"reported_health":"reported_health_label","capacity_retention_percent":"qualified_learned_full_over_design","reported_cycles":"reported_counter_semantics_require_qualification","observed_efc":"observed_discharge_only_with_coverage"},"physical_charge_stopped":null})";
}
int policy_command(int argc, char** argv) {
    if (argc < 3 || std::string_view(argv[1]) != "charge") return -1;
    if (argc == 3 && std::string_view(argv[2]) == "modes") {
        std::cout << policy_modes_json() << '\n'; return 0;
    }
    if (argc != 4 || (std::string_view(argv[2]) != "plan" && std::string_view(argv[2]) != "mode")) return -1;
    const std::string name = argv[3];
    const auto request = preset(name);
    if (!request) throw std::runtime_error("Charge mode must be firmware, charge80, hold80-90, slow or automatic");
    // Static provider capability is intentionally not inferred from file existence,
    // model attestation, plausible readings or operator-selected preferences.
    Planner planner;
    const auto plan = planner.evaluate(*request, {}, {}, {});
    const bool apply = std::string_view(argv[2]) == "mode";
    std::cout << "{\"schema_version\":1,\"kind\":\"uke_charge_policy_plan\",\"requested\":\"" << name
        << "\",\"operation\":\"" << (apply ? "apply" : "preview")
        << "\",\"state\":\"" << (apply ? "unsupported" : plan.state)
        << "\",\"reason\":\"" << (apply ? "qualified_actuator_not_implemented" : plan.reason)
        << "\",\"effective\":null,\"applied\":false,\"hardware_write\":false,\"persisted\":false,\"physical_charge_stopped\":null}" << '\n';
    return apply ? 69 : 0;
}
}
