// SPDX-License-Identifier: MIT
// Replaces only query/request entry points in the CLI process, never hardware IO.
#include "backend.hpp"
namespace uke {
Result battery_query(const std::string& operation){return {operation=="protocol"?69:0,"{\"view\":\""+operation+"\",\"individual_cell_health\":null,\"reported_health\":\"Good\"}\n"};}
std::string nfc_status(){return "{\"available\":false,\"powered\":null,\"polling_started\":false}\n";}
Result charge_control_mode(const std::string&){return {69,"{\"applied\":false,\"effective\":null,\"reason\":\"control_backend_unqualified\"}\n"};}
Result charge_review(const std::string&){return {69,"{\"identity_installed\":false,\"reason\":\"control_profile_unqualified\"}\n"};}
Result usb_request_data(const std::string& role){return {0,"{\"requested_role\":\""+role+"\",\"physical_acceptance\":null}\n"};}
std::string status(){return "{\"schema\":\"org.uke.control.status.v1\",\"swap\":{\"active\":[]},\"charge\":{\"available\":false},\"usb\":{\"available\":false}}\n";}
}
