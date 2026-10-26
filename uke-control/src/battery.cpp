// SPDX-License-Identifier: MIT
#include "battery.hpp"
#include "sysfs.hpp"
#include <array>
#include <cstdlib>
#include <memory>
#include <systemd/sd-json.h>
namespace uke::battery {
namespace {
std::string json(sd_json_variant* value) {
    if(!value)return "null";
    char* text=nullptr;
    if(sd_json_variant_format(value,static_cast<sd_json_format_flags_t>(0),&text)<0||!text)throw std::runtime_error("Cannot format battery report");
    std::unique_ptr<char,decltype(&std::free)> owned(text,std::free);return text;
}
sd_json_variant* field(sd_json_variant* value,const char* key){return sd_json_variant_is_object(value)?sd_json_variant_by_key(value,key):nullptr;}
bool yes(sd_json_variant* value){return sd_json_variant_is_boolean(value)&&sd_json_variant_boolean(value);}
}
std::string view(const std::string& operation,const std::string& document) {
    if(document.size()>65536||document.find('\0')!=document.npos)throw std::runtime_error("Invalid battery report size");
    sd_json_variant* raw=nullptr;
    if(sd_json_parse(document.c_str(),static_cast<sd_json_parse_flags_t>(0),&raw,nullptr,nullptr)<0)throw std::runtime_error("Battery backend returned malformed data");
    std::unique_ptr<sd_json_variant,decltype(&sd_json_variant_unref)> parsed(raw,sd_json_variant_unref);
    if(!sd_json_variant_is_object(raw))throw std::runtime_error("Battery backend did not return an object");
    std::string out="{\"schema\":\"org.uke.control.battery.v1\",\"view\":"+sysfs::quote(operation);
    if(operation=="protocol") {
        auto kind=field(raw,"kind");
        if(!sd_json_variant_is_string(kind)||std::string(sd_json_variant_string(kind))!="uke_charging_protocol_report")throw std::runtime_error("Unexpected charging protocol backend");
        auto cache=field(raw,"cached_observation");auto type=field(cache,"normalized_standard_type");
        constexpr std::array names{"Unknown","USB SDP","USB DCP","USB CDP","USB ACA","USB Type-C","USB Power Delivery (revision not reported)","USB Power Delivery DRP (revision not reported)","USB Power Delivery PPS (revision not reported)"};
        const bool fresh=yes(field(raw,"metadata_available"))&&yes(field(cache,"fresh"))&&yes(field(cache,"cache_matches_epochs"));
        const bool known=fresh&&sd_json_variant_is_unsigned(type)&&sd_json_variant_unsigned(type)>0&&sd_json_variant_unsigned(type)<names.size();
        out+=",\"source\":\"uke-charge-control protocols\",\"fresh\":"+std::string(fresh?"true":"false")+",\"reported_usb_type\":"+(known?sysfs::quote(names[sd_json_variant_unsigned(type)]):"null");
        for(const auto* key:{"negotiated_protocol","negotiated_pd_revision","reason"})out+=','+sysfs::quote(key)+':'+(!fresh&&std::string_view(key)!="reason"?"null":json(field(raw,key)));
        return out+",\"reported_usb_type_is_negotiated_version\":false}\n";
    }
    if(operation!="status"&&operation!="health"&&operation!="cells")throw std::runtime_error("Unknown battery report");
    auto version=field(raw,"schema_version");
    auto kind=field(raw,"kind");
    if(!sd_json_variant_is_unsigned(version)||sd_json_variant_unsigned(version)!=1||!sd_json_variant_is_string(kind)||std::string_view(sd_json_variant_string(kind))!="uke_battery_snapshot"||!sd_json_variant_is_string(field(raw,"status")))throw std::runtime_error("Unexpected battery observer schema");
    for(const auto* key:{"status","provider_verified","snapshot_coherent","receipt_boottime_ns","sample_end_boottime_ns"})out+=','+sysfs::quote(key)+':'+json(field(raw,key));
    const bool coherent=yes(field(raw,"provider_verified"))&&yes(field(raw,"snapshot_coherent"));
    auto fields=coherent?field(raw,"fields"):nullptr;auto electrical=coherent?field(raw,"electrical"):nullptr;
    if(operation=="status") {
        out+=",\"readings\":{";bool first=true;
        for(const auto* key:{"battery.status","battery.present","battery.capacity","battery.voltage_now","battery.current_now","battery.temp","usb.online","usb.usb_type"}){if(!first)out+=',';first=false;out+=sysfs::quote(key)+':'+json(field(fields,key));}
        out+='}';
    }
    if(operation!="cells") {
        for(const auto* key:{"health","health_provenance"})out+=','+sysfs::quote(key)+':'+json(coherent?field(raw,key):nullptr);
        for(const auto* key:{"battery.cycle_count","battery.charge_full","battery.charge_full_design"})out+=','+sysfs::quote(key)+':'+json(field(fields,key));
    }
    // Pack health and equal-share estimates do not establish individual cell health.
    out+=",\"reported_cell_topology\":"+json(field(electrical,"cell_topology"));
    return out+",\"individual_cell_measurement\":false,\"individual_cell_health\":null,\"cell_health_reason\":\"individual_cell_health_not_exported_by_battery_backend\"}\n";
}
}
