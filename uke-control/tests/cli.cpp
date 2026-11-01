// SPDX-License-Identifier: MIT
#include "cli.hpp"
#include "sysfs.hpp"
#include <cassert>
#include <iostream>
#include <sstream>
#include <systemd/sd-json.h>
using namespace uke::cli;
static Arguments args(std::initializer_list<const char*> input) {
    std::vector<char*> values;for(auto* word:input)values.push_back(const_cast<char*>(word));return arguments(values.size(),values.data());
}
static std::string output(const std::string& data,bool json) {
    std::ostringstream result;auto* old=std::cout.rdbuf(result.rdbuf());
    try{print(data,json);std::cout.rdbuf(old);}catch(...){std::cout.rdbuf(old);throw;}
    return result.str();
}
int main() {
    assert(uke::sysfs::quote(std::string("a\0b",3))=="\"a\\u0000b\"");
    for(const auto& value:{std::string("Türkçe 😀 中文 e\xcc\x81"),std::string("a\nb\t\"\\")}) {
        const auto quoted=uke::sysfs::quote(value);
        sd_json_variant* parsed=nullptr;
        assert(sd_json_parse(quoted.c_str(),sd_json_parse_flags_t(0),&parsed,nullptr,nullptr)>=0);
        assert(value==sd_json_variant_string(parsed));
        sd_json_variant_unref(parsed);
    }
    for(const auto& invalid:{std::string("\x80"),std::string("\xc0\xaf"),std::string("\xed\xa0\x80"),
                            std::string("\xf4\x90\x80\x80"),std::string("\xe2\x82")}) {
        const auto quoted=uke::sysfs::quote(invalid);
        assert(quoted.find("\\ufffd")!=std::string::npos);
        sd_json_variant* parsed=nullptr;
        assert(sd_json_parse(quoted.c_str(),sd_json_parse_flags_t(0),&parsed,nullptr,nullptr)>=0);
        sd_json_variant_unref(parsed);
    }
    const auto renamed=args({"ignored","--json","brunch","test"});
    assert(renamed.json && (renamed.words==std::vector<std::string>{"uke-control","channel","set","test"}));
    assert(args({"app","branch","status","--json"}).json);
    assert(args({"app","diagnostic","wifi"}).words[1]=="diagnostics");
    assert((args({"app","set","cdc","on","--persistent"}).words==std::vector<std::string>{"uke-control","cdc","on","--persistent"}));
    assert((args({"app","usb","request","data","host"}).words==std::vector<std::string>{"uke-control","usb","role","host"}));
    auto after=args({"app","report","github","--","--json","repo"});assert(!after.json && after.words[3]=="--json");
    assert(args({"app","charge","mode","slow","-h"}).help);
    assert(args({"app","resize","system","--version"}).version);
    bool refused=false;try{args({"app","--json","--json"});}catch(const UsageError&){refused=true;}assert(refused);
    for(const auto* group:{"status","channel","update","swap","inventory","charge","usb","cdc","ethernet","display","desktop","hardware","firmware","battery","nfc","wake","flashlight","led","sensors","diagnostics","report","resize","test"})assert(help(group).find("Usage: uke-control ") == 0);
    assert(help("display").find("display compression [auto|on|off]")!=std::string::npos);
    refused=false;try{help("unknown");}catch(const UsageError&){refused=true;}assert(refused);
    const std::string data=R"({"schema":"org.uke.control.fixture.v1","flashlight":{"available":true,"on":false,"requested_percent":17,"brightness_percent":0,"max_microamp":20000},"physical_acceptance":null,"values":[true,false,null,1.25],"reason":"control_backend_unqualified","detail":"{\"applied\":false}","warning":"\u001b[31munsafe\rtext"})";
    const auto text=output(data,false);
    assert(text.find("Flashlight:\n  Available: yes")!=std::string::npos);
    assert(text.find("Requested brightness (%): 17")!=std::string::npos);
    assert(text.find("Maximum current (µA): 20000")!=std::string::npos);
    assert(text.find("Physical validation: not established")!=std::string::npos);
    assert(text.find("control backend unqualified")!=std::string::npos);
    assert(text.find("Applied: no")!=std::string::npos);
    assert(text.find('{')==std::string::npos && text.find("org.uke.control")==std::string::npos);
    assert(text.find('\x1b')==std::string::npos && text.find('\r')==std::string::npos && text.find("\\x1b")!=std::string::npos);
    assert(output(data,true)==data+'\n');
    const std::string torch=R"({"schema":"org.uke.control.lights.v1","physical_acceptance":null,"flashlight":{"available":true,"brightness_percent":50},"rgb":{"available":false,"reason":"unrelated"}})";
    assert(human(torch).find("unrelated")==std::string::npos && human(torch).find("Applied brightness (%): 50")!=std::string::npos);
    const auto wrapped=output("report directory\n",true);sd_json_variant* parsed=nullptr;
    assert(sd_json_parse(wrapped.c_str(),static_cast<sd_json_parse_flags_t>(0),&parsed,nullptr,nullptr)>=0);
    assert(std::string(sd_json_variant_string(sd_json_variant_by_key(parsed,"output")))=="report directory\n");sd_json_variant_unref(parsed);
    assert(human("Request failed:\n{\"applied\":false}\n")=="Request failed:\nApplied: no\n");
    refused=false;try{human("{broken JSON");}catch(const std::exception&){refused=true;}assert(refused);
    std::cout<<"CLI argument boundaries, legacy aliases, help, readable nested output, unknown values, control-byte escaping and explicit JSON pass (host only)\n";
}
