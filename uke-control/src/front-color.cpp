// SPDX-License-Identifier: MIT
#include "front-color.hpp"
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fcntl.h>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>
#include <sys/stat.h>
#include <systemd/sd-json.h>
#include <time.h>
#include <unistd.h>

namespace uke::display {
namespace {
constexpr std::uint64_t sample_window=30'000'000'000ULL, publisher_window=5'000'000'000ULL;
void require(bool value) { if(!value) throw std::runtime_error("invalid_snapshot"); }
struct Json { sd_json_variant *p=nullptr; ~Json(){sd_json_variant_unref(p);} };
struct Fd { int value=-1; ~Fd(){if(value>=0)close(value);} };
std::string unavailable(const char *reason) {
    return std::string("{\"schema\":\"org.uke.control.front-color.v1\",\"version\":1,\"available\":false,\"reader_reason\":\"")+
        reason+"\",\"values\":[],\"units\":\"unverified\",\"calibration\":\"unverified\"}\n";
}
std::uint64_t number(sd_json_variant *v) {
    require(sd_json_variant_is_unsigned(v)); return sd_json_variant_unsigned(v);
}
bool boolean(sd_json_variant *v) {
    require(sd_json_variant_is_boolean(v)); return sd_json_variant_boolean(v);
}
std::string_view string(sd_json_variant *v) {
    require(sd_json_variant_is_string(v)); return sd_json_variant_string(v);
}
bool trusted(int fd, bool directory) {
    struct stat st{};
    return fd>=0 && fstat(fd,&st)==0 && st.st_uid==0 && !(st.st_mode&0022) &&
        (directory ? S_ISDIR(st.st_mode)&&(st.st_mode&07777)==0755 :
            S_ISREG(st.st_mode)&&(st.st_mode&07777)==0644&&st.st_nlink==1&&st.st_size>0&&st.st_size<=4096);
}
}
std::string front_color_snapshot(std::string_view payload, std::uint64_t now_ns) {
    try {
        require(!payload.empty()&&payload.size()<=4096&&payload.find('\0')==std::string_view::npos);
        Json doc; const std::string input(payload);
        require(sd_json_parse(input.c_str(),static_cast<sd_json_parse_flags_t>(0),&doc.p,nullptr,nullptr)>=0);
        require(sd_json_variant_is_object(doc.p));
        const std::set<std::string> expected{"schema","version","data_type","connected","has_sample","fresh",
            "available","error","values","status","timestamp_ticks","dsp_clock_hz","dsp_clock_mapping",
            "received_boottime_ns","published_boottime_ns","freshness_window_ns","units","calibration"};
        std::set<std::string> keys;
        require(sd_json_variant_elements(doc.p)==expected.size()*2);
        for(std::size_t i=0;i<sd_json_variant_elements(doc.p);i+=2)
            require(keys.emplace(string(sd_json_variant_by_index(doc.p,i))).second);
        require(keys==expected);
        const auto field=[&](const char *key){return sd_json_variant_by_key(doc.p,key);};
        require(string(field("schema"))=="org.uke.sensors.front-color.v1" && number(field("version"))==1 &&
            string(field("data_type"))=="cct_front" && string(field("units"))=="unverified" &&
            string(field("calibration"))=="unverified" && string(field("dsp_clock_mapping"))=="unverified" &&
            sd_json_variant_is_null(field("dsp_clock_hz")) && number(field("freshness_window_ns"))==sample_window);
        const bool connected=boolean(field("connected")), has=boolean(field("has_sample"));
        const bool published_fresh=boolean(field("fresh")), published_available=boolean(field("available"));
        const auto error=string(field("error"));
        const std::set<std::string_view> errors{"","starting","awaiting_sample","metadata_unavailable","transport_unavailable","invalid_sample"};
        require(errors.contains(error));
        auto values=field("values"); require(sd_json_variant_is_array(values));
        const auto count=sd_json_variant_elements(values);
        require(count<=16 && has==(count!=0));
        for(std::size_t i=0;i<count;i++) {
            auto v=sd_json_variant_by_index(values,i); require(sd_json_variant_is_real(v));
            auto value=sd_json_variant_real(v);
            require(std::isfinite(value)&&std::abs(value)<=std::numeric_limits<float>::max());
        }
        const auto published=number(field("published_boottime_ns"));
        require(published<=now_ns);
        std::uint64_t received=0, status=0;
        if(has) {
            require(connected&&error.empty());
            received=number(field("received_boottime_ns")); status=number(field("status"));
            require(received<=published && number(field("timestamp_ticks"))>0 && status<=3);
        } else {
            require(sd_json_variant_is_null(field("received_boottime_ns")) && sd_json_variant_is_null(field("status")) &&
                sd_json_variant_is_null(field("timestamp_ticks")) && !error.empty());
            require(connected ? error=="awaiting_sample" : error!="awaiting_sample");
        }
        const bool was_fresh=connected&&has&&published-received<=sample_window;
        require(published_fresh==was_fresh && published_available==(was_fresh&&status>0&&error.empty()));
        const bool alive=now_ns-published<=publisher_window;
        const bool fresh=connected&&has&&now_ns-received<=sample_window;
        const bool available=alive&&fresh&&status>0&&error.empty();
        const char *reason=!alive?"publisher_stale":!connected?"disconnected":!has?"awaiting_sample":
            !fresh?"sample_stale":status==0?"sample_unreliable":"";
        require(sd_json_variant_set_field_boolean(&doc.p,"fresh",fresh)>=0 &&
            sd_json_variant_set_field_boolean(&doc.p,"color_application_ready",false)>=0 &&
            sd_json_variant_set_field_string(&doc.p,"color_application_reason","units_and_calibration_unverified")>=0 &&
            sd_json_variant_set_field_boolean(&doc.p,"available",available)>=0 &&
            sd_json_variant_set_field_boolean(&doc.p,"publisher_alive",alive)>=0 &&
            sd_json_variant_set_field_unsigned(&doc.p,"reader_boottime_ns",now_ns)>=0 &&
            sd_json_variant_set_field_string(&doc.p,"reader_reason",reason)>=0 &&
            sd_json_variant_set_field_string(&doc.p,"source_schema","org.uke.sensors.front-color.v1")>=0 &&
            sd_json_variant_set_field_string(&doc.p,"schema","org.uke.control.front-color.v1")>=0);
        char *formatted=nullptr;
        require(sd_json_variant_format(doc.p,SD_JSON_FORMAT_NEWLINE,&formatted)>=0 && formatted);
        const std::unique_ptr<char,decltype(&free)> output(formatted,free);
        return output.get();
    } catch(const std::exception&) { return unavailable("invalid_snapshot"); }
}
std::string front_color_status() {
    // Traverse trusted runtime directories without following a redirected leaf.
    Fd run{open("/run",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW)};
    if(!trusted(run.value,true))return unavailable("untrusted_path");
    Fd base{openat(run.value,"uke-sensors",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW)};
    if(!trusted(base.value,true))return unavailable("not_publishing");
    Fd input{openat(base.value,"front-color.json",O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW)};
    if(!trusted(input.value,false))return unavailable("not_publishing");
    std::array<char,4097> buffer{};std::size_t used=0;
    for(;;) {
        auto count=::read(input.value,buffer.data()+used,buffer.size()-used);
        if(count<0&&errno==EINTR)continue;
        if(count<0)return unavailable("read_failed");
        if(!count)break;
        used+=static_cast<std::size_t>(count);
        if(used>4096)return unavailable("invalid_snapshot");
    }
    timespec now{};
    if(clock_gettime(CLOCK_BOOTTIME,&now)!=0 || now.tv_sec<0)return unavailable("clock_unavailable");
    const auto now_ns=std::uint64_t(now.tv_sec)*1'000'000'000ULL+std::uint64_t(now.tv_nsec);
    return front_color_snapshot(std::string_view(buffer.data(),used),now_ns);
}
}
