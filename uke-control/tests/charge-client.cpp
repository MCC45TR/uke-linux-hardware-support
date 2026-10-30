// SPDX-License-Identifier: MIT
#include "charge-client.hpp"
#include <cassert>
#include <deque>
#include <iostream>
#include <stdexcept>

namespace {
std::deque<uke::Result> responses;
std::vector<std::vector<std::string>> calls;
void reset(std::initializer_list<uke::Result> results) {responses=results;calls.clear();}
}
namespace uke {
Result execute(const std::vector<std::string>& arguments, unsigned seconds, size_t limit) {
    assert(seconds>0&&seconds<=40&&limit<=65536);
    assert(!responses.empty());calls.push_back(arguments);
    auto result=responses.front();responses.pop_front();return result;
}
}
int main() {
    using uke::charge::control_mode;
    reset({{0,"{\"operation\":\"metadata_only\",\"physical_charge_stopped\":null}\n"}});
    const auto status=uke::charge::control_query("status");
    assert(status.status==0&&status.output.find("metadata_only")!=std::string::npos);
    assert((calls==std::vector<std::vector<std::string>>{{"/usr/libexec/uke-charge-control","status"}}));
    reset({{69,"metadata unavailable"}});
    assert(uke::charge::control_query("status").status==69&&calls.size()==1);
    reset({{0,"{\"kind\":\"uke_charge_settings\",\"applied\":false}\n"}});
    assert(uke::charge::control_query("settings").status==0);
    assert((calls==std::vector<std::vector<std::string>>{{"/usr/libexec/uke-charge-control","settings"}}));
    reset({{69,"protocol diagnostics disabled"}});
    assert(uke::charge::control_query("protocol-inspect").status==69);
    assert((calls==std::vector<std::vector<std::string>>{{"/usr/libexec/uke-charge-control","protocol-inspect"}}));
    reset({{0,"{\"kind\":\"uke_charge_protection_cache\",\"fresh\":true,\"reported_moisture_detection_enabled\":false,\"reported_moisture\":false,\"wet_latched\":false}"}});
    assert(uke::charge::control_query("protection-status").status==0);
    assert((calls==std::vector<std::vector<std::string>>{{"/usr/libexec/uke-charge-control","protection-status"}}));
    for (auto enabled:{false,true}) for(auto fresh:{false,true}) for(auto wet:{false,true}) {
        reset({{0,std::string("{\"kind\":\"uke_charge_protection_cache\",\"fresh\":")+(fresh?"true":"false")+
            ",\"reported_moisture_detection_enabled\":"+(enabled?"true":"false")+
            ",\"reported_moisture\":"+(wet?"true":"false")+",\"wet_latched\":false}"}});
        const auto report=uke::charge::control_query("protection-status");
        assert(report.status==0);
        const auto expected=!fresh?"unknown_stale":wet?"wet_reported":!enabled?"unknown_detector_disabled":"dry_reported";
        assert(report.output.find(expected)!=std::string::npos);
        assert(report.output.find("\"physical_protection_qualified\":false")!=std::string::npos);
    }
    reset({{0,"invalid protection"}});
    assert(uke::charge::control_query("protection-status").status==69);
    reset({{0,"moisture enabled"}});
    assert(uke::charge::enable_moisture().status==0);
    assert((calls==std::vector<std::vector<std::string>>{{"/usr/libexec/uke-charge-control","moisture-enable"}}));
    reset({});
    bool mutation_rejected=false;
    try{uke::charge::control_query("moisture-enable");}catch(const std::exception&){mutation_rejected=true;}
    assert(mutation_rejected&&calls.empty());
    reset({});
    bool query_rejected=false;
    try{uke::charge::control_query("verify");}catch(const std::exception&){query_rejected=true;}
    assert(query_rejected&&calls.empty());
    reset({{0,"{\"request_sent\":true,\"applied\":null}\n"}});
    const auto applied=uke::charge::apply_preconfigured_cap();
    assert(applied.status==0&&applied.output.find("\"applied\":null")!=std::string::npos);
    assert((calls==std::vector<std::vector<std::string>>{{"/usr/libexec/uke-charge-control","apply-cap80"}}));
    reset({{69,"uncertain"}});
    assert(uke::charge::apply_preconfigured_cap().status==69&&calls.size()==1);
    reset({});query_rejected=false;
    try{uke::charge::control_query("apply-cap80");}catch(const std::exception&){query_rejected=true;}
    assert(query_rejected&&calls.empty());
    reset({{69,"restricted cap unavailable"}});
    assert(uke::charge::enable_preconfigured_cap().status==69);
    assert((calls==std::vector<std::vector<std::string>>{{"/usr/libexec/uke-charge-control","enable-cap80"}}));
    reset({});query_rejected=false;
    try{uke::charge::control_query("enable-cap80");}catch(const std::exception&){query_rejected=true;}
    assert(query_rejected&&calls.empty());
    reset({{0,"{\"applied\":true}\n"}});
    assert(control_mode("charge80").status==0);
    assert((calls==std::vector<std::vector<std::string>>{{"/usr/libexec/uke-charge-control","cap80"}}));
    reset({{69,"unqualified"}});assert(control_mode("hold80-90").status==69);
    assert((calls==std::vector<std::vector<std::string>>{{"/usr/libexec/uke-charge-control","band80-90"}}));
    reset({{69,"unqualified"}});assert(control_mode("hold70-80").status==69);
    assert((calls==std::vector<std::vector<std::string>>{{"/usr/libexec/uke-charge-control","pair","70","80"}}));
    reset({});assert(control_mode("temporary-full").status==69&&calls.empty());
    for(const char* reply:{"{}", "bad json", "{\"kind\":\"foreign\",\"automatic_admission_ready\":true}",
        "{\"kind\":\"uke_charge_control_admission\",\"automatic_admission_ready\":false}",
        "{\"kind\":\"uke_charge_control_admission\",\"automatic_admission_ready\":\"true\"}"}) {
        reset({{0,reply}});assert(control_mode("automatic").status==69);assert(calls.size()==1);
    }
    const std::string automatic="{\"kind\":\"uke_charge_control_admission\",\"automatic_admission_ready\":true}";
    reset({{69,automatic}});assert(control_mode("automatic").status==69&&calls.size()==1);
    reset({{0,automatic+std::string("\0garbage",8)}});assert(control_mode("automatic").status==69&&calls.size()==1);
    reset({{0,automatic},{1,"start failure"}});assert(control_mode("automatic").status==69&&calls.size()==2);
    reset({{0,automatic},{0,""},{0,"ActiveState=failed\nSubState=failed\n"}});
    assert(control_mode("automatic").status==69);
    reset({{0,automatic},{0,""},{0,"SubState=running\nActiveState=active\n"}});
    auto started=control_mode("automatic");assert(started.status==0&&started.output.find("\"applied\":null")!=std::string::npos);
    assert((calls[1]==std::vector<std::string>{"/usr/bin/systemctl","start","uke-charge-automatic.service"}));
    const std::string qualified="{\"kind\":\"uke_charge_control_admission\",\"profile_qualified\":true}";
    reset({{0,qualified},{1,"stop failed"}});assert(control_mode("slow").status==69&&calls.size()==2);
    reset({{0,qualified},{0,""},{0,"applied"}});assert(control_mode("slow").status==0&&calls.size()==3);
    assert(calls[1][1]=="stop"&&calls[2][1]=="slow");
    reset({{0,qualified},{1,"stop failed"}});assert(control_mode("profile-restore").status==69&&calls.size()==2);
    reset({{0,qualified},{0,""},{69,"restore failed"}});
    assert(control_mode("profile-restore").status==69&&calls.size()==3&&calls[2][1]=="profile-restore");
    reset({{0,qualified},{0,""},{0,"verified positive FCC"}});
    assert(control_mode("profile-restore").status==0&&calls[2][1]=="profile-restore");
    reset({});assert(control_mode("firmware").status==69&&calls.empty());
    bool rejected=false;try{control_mode("raw-voltage");}catch(const std::exception&){rejected=true;}
    assert(rejected&&calls.empty());
    std::cout<<"Charge-client admission, failed-service handoff and applied-state boundaries passed; no device operation\n";
}
