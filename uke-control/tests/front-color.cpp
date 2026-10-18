// SPDX-License-Identifier: MIT
#include "../src/front-color.cpp"
#include <cassert>
#include <initializer_list>
#include <filesystem>
#include <memory>
#include <string>
#include <systemd/sd-json.h>
using uke::display::front_color_snapshot;
static std::string replace(std::string value,const std::string& from,const std::string& to) {
    auto at=value.find(from); assert(at!=std::string::npos); value.replace(at,from.size(),to); return value;
}
static bool available(const std::string& value) {
    sd_json_variant *p=nullptr;
    assert(sd_json_parse(value.c_str(),static_cast<sd_json_parse_flags_t>(0),&p,nullptr,nullptr)>=0);
    std::unique_ptr<sd_json_variant,decltype(&sd_json_variant_unref)> doc(p,sd_json_variant_unref);
    auto flag=sd_json_variant_by_key(p,"available"); assert(sd_json_variant_is_boolean(flag));
    return sd_json_variant_boolean(flag);
}
int main() {
    char pattern[]="/tmp/uke-front-color-reader-XXXXXX";
    auto name=mkdtemp(pattern);assert(name);
    const std::filesystem::path fixture=name;
    {
        using uke::display::trusted;
        uke::display::Fd directory{open(name,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW)};
        assert(directory.value>=0);
        for(auto mode : {0700,0750,0775,01755}) {
            assert(chmod(name,mode)==0);assert(!trusted(directory.value,true));
        }
        assert(chmod(name,0755)==0);
        assert(trusted(directory.value,true)==(geteuid()==0));
        auto file=fixture/"front-color.json";
        uke::display::Fd fd{open(file.c_str(),O_CREAT|O_EXCL|O_RDWR|O_CLOEXEC,0600)};
        assert(fd.value>=0 && write(fd.value,"{}",2)==2);
        assert(!trusted(fd.value,false));
        assert(fchmod(fd.value,0644)==0 && trusted(fd.value,false)==(geteuid()==0));
        if(geteuid()==0) {
            assert(fchown(fd.value,1,0)==0&&!trusted(fd.value,false));
            assert(fchown(fd.value,0,0)==0&&trusted(fd.value,false));
        }
        assert(!trusted(fd.value,true)&&!trusted(directory.value,false));
        assert(link(file.c_str(),(fixture/"hardlink").c_str())==0);
        assert(!trusted(fd.value,false));assert(unlink((fixture/"hardlink").c_str())==0);
        assert(ftruncate(fd.value,4097)==0 && !trusted(fd.value,false));
        assert(ftruncate(fd.value,0)==0 && !trusted(fd.value,false));
        assert(symlink("front-color.json",(fixture/"link").c_str())==0);
        assert(openat(directory.value,"link",O_RDONLY|O_CLOEXEC|O_NOFOLLOW)<0);
        assert(mkfifo((fixture/"fifo").c_str(),0644)==0);
        uke::display::Fd fifo{openat(directory.value,"fifo",O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW)};
        assert(fifo.value>=0&&!trusted(fifo.value,false));
    }
    std::filesystem::remove_all(fixture);
    const std::string sample=R"({"schema":"org.uke.sensors.front-color.v1","version":1,"data_type":"cct_front","connected":true,"has_sample":true,"fresh":true,"available":true,"error":"","values":[4500],"status":3,"timestamp_ticks":123,"dsp_clock_hz":null,"dsp_clock_mapping":"unverified","received_boottime_ns":10000000000,"published_boottime_ns":11000000000,"freshness_window_ns":30000000000,"units":"unverified","calibration":"unverified"})";
    auto inspected=front_color_snapshot(sample,12'000'000'000);
    assert(available(inspected) && inspected.find("org.uke.control.front-color.v1")!=std::string::npos);
    assert(available(front_color_snapshot(sample,16'000'000'000)));
    assert(!available(front_color_snapshot(sample,16'000'000'001)));
    assert(!available(front_color_snapshot(sample,9'000'000'000)));
    auto old=replace(sample,"11000000000","39000000000");
    assert(available(front_color_snapshot(old,40'000'000'000)));
    inspected=front_color_snapshot(old,40'000'000'001);
    assert(!available(inspected) && inspected.find("sample_stale")!=std::string::npos);
    const auto unreliable=replace(replace(sample,"\"status\":3","\"status\":0"),"\"available\":true","\"available\":false");
    inspected=front_color_snapshot(unreliable,12'000'000'000);
    assert(!available(inspected)&&inspected.find("sample_unreliable")!=std::string::npos);
    for(const auto& malformed : {
        replace(sample,"\"version\":1","\"version\":2"),
        replace(sample,"\"version\":1","\"version\":1,\"version\":1"),
        replace(sample,"\"units\":\"unverified\"","\"units\":\"kelvin\""),
        replace(sample,"\"dsp_clock_hz\":null","\"dsp_clock_hz\":19200000"),
        replace(sample,"[4500]","[\"4500\"]"), replace(sample,"[4500]","[1e999]"),
        replace(sample,"[4500]","[]"), replace(sample,"\"status\":3","\"status\":4"),
        replace(sample,"\"connected\":true","\"connected\":false"),
        replace(sample,"\"fresh\":true","\"fresh\":false"),
        replace(sample,"\"timestamp_ticks\":123","\"timestamp_ticks\":0"),
        replace(sample,"10000000000","12000000000"),
        replace(sample,"\"has_sample\":true","\"has_sample\":false"),
        sample+std::string(1,'\0'), std::string(4097,' '),std::string("{}")}) {
        inspected=front_color_snapshot(malformed,12'000'000'000);
        assert(!available(inspected)&&inspected.find("invalid_snapshot")!=std::string::npos);
    }
    auto waiting=replace(sample,"\"has_sample\":true","\"has_sample\":false");
    waiting=replace(waiting,"\"fresh\":true","\"fresh\":false");
    waiting=replace(waiting,"\"available\":true","\"available\":false");
    waiting=replace(waiting,"\"error\":\"\"","\"error\":\"awaiting_sample\"");
    waiting=replace(waiting,"[4500]","[]");
    waiting=replace(waiting,"\"status\":3","\"status\":null");
    waiting=replace(waiting,"\"timestamp_ticks\":123","\"timestamp_ticks\":null");
    waiting=replace(waiting,"\"received_boottime_ns\":10000000000","\"received_boottime_ns\":null");
    inspected=front_color_snapshot(waiting,12'000'000'000);
    assert(!available(inspected)&&inspected.find("awaiting_sample")!=std::string::npos);
}
