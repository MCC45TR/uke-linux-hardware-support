// SPDX-License-Identifier: MIT
#include "../src/battery.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
template<class F>void refuse(F f){bool bad=false;try{f();}catch(const std::exception&){bad=true;}assert(bad);}
int main(){
    using uke::battery::view;
    const std::string data=R"({"schema_version":1,"kind":"uke_battery_snapshot","status":"source_unqualified","provider_verified":true,"snapshot_coherent":true,"fields":{"battery.health":{"value":"Good"},"battery.cycle_count":{"value":20,"validity":"unqualified"}},"health":{"capacity_retention_percent":null},"health_provenance":{"reported_health":{"reported":"Good","physical_safety_qualified":false}},"engineering":{"cell_topology":{"series_cells":2,"individual_cell_measurement":false,"cell_imbalance_uv":null}}})";
    auto health=view("health",data);assert(health.find("\"reported\":\"Good\"")!=health.npos&&health.find("\"individual_cell_health\":null")!=health.npos&&health.find("\"capacity_retention_percent\":null")!=health.npos);
    auto cells=view("cells",data);assert(cells.find("\"series_cells\":2")!=cells.npos&&cells.find("\"individual_cell_measurement\":false")!=cells.npos&&cells.find("Good")==cells.npos);
    const auto unavailable=view("health",R"({"schema_version":1,"kind":"uke_battery_snapshot","status":"provider_unverified","provider_verified":false})");
    assert(unavailable.find("\"health\":null")!=unavailable.npos);
    const std::string protocol=R"({"kind":"uke_charging_protocol_report","metadata_available":true,"cached_observation":{"fresh":true,"cache_matches_epochs":true,"normalized_standard_type":6},"negotiated_protocol":null,"negotiated_pd_revision":null,"reason":"cached_legacy_wire6_not_protocol_qualification"})";
    auto pd=view("protocol",protocol);assert(pd.find("USB Power Delivery (revision not reported)")!=pd.npos&&pd.find("\"negotiated_pd_revision\":null")!=pd.npos);
    auto stale=protocol;stale.replace(stale.find("\"fresh\":true"),12,"\"fresh\":false");assert(view("protocol",stale).find("\"reported_usb_type\":null")!=pd.npos);
    stale.insert(stale.size()-1,",\"pd_contract\":{\"available\":true,\"pdo\":184620,\"rdo\":268640506,\"electrical_measurement\":false}");
    assert(view("protocol",stale).find("\"pdo\":184620")!=pd.npos&&view("protocol",stale).find("\"electrical_measurement\":false")!=pd.npos);
    auto invalid=protocol;invalid.replace(invalid.find("\"normalized_standard_type\":6"),28,"\"normalized_standard_type\":99");assert(view("protocol",invalid).find("\"reported_usb_type\":null")!=pd.npos);
    refuse([]{view("health","[]");});refuse([]{view("health","bad JSON");});refuse([&]{view("health",data+std::string("\0private",8));});
    refuse([&]{view("protocol",data);});refuse([&]{view("unknown",data);});
    std::cout<<"Battery reports: health provenance, absent per-cell data, USB class without revision inference and malformed/stale input checks passed\n";
}
