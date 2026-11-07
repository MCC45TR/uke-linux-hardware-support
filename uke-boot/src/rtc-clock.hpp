// SPDX-License-Identifier: MIT
#pragma once
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <fcntl.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

namespace uke_rtc {
namespace fs=std::filesystem;
inline constexpr const char *node="/sys/firmware/devicetree/base/soc/qcom,spmi@c42d000/qcom,pmk8550@0/rtc@6100";
inline void require(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
inline void bounds(int64_t utc) {require(utc>=1577836800 && utc<4102444800,"RTC UTC is outside 2020-2099");}
inline int64_t distance(int64_t a,int64_t b) {return a>b?a-b:b-a;}
struct Sample {int64_t utc;bool synchronized;};
enum class Result {deferred,correct,written};
inline int64_t utc(const rtc_time &value) {
    require(value.tm_year>=120 && value.tm_year<200 && value.tm_mon>=0 && value.tm_mon<12 &&
            value.tm_mday>=1 && value.tm_mday<=31 && value.tm_hour>=0 && value.tm_hour<24 &&
            value.tm_min>=0 && value.tm_min<60 && value.tm_sec>=0 && value.tm_sec<60,"Invalid RTC calendar");
    std::tm input{};
    input.tm_year=value.tm_year;input.tm_mon=value.tm_mon;input.tm_mday=value.tm_mday;
    input.tm_hour=value.tm_hour;input.tm_min=value.tm_min;input.tm_sec=value.tm_sec;
    const auto epoch=timegm(&input);std::tm check{};
    require(gmtime_r(&epoch,&check) && check.tm_year==value.tm_year && check.tm_mon==value.tm_mon &&
            check.tm_mday==value.tm_mday && check.tm_hour==value.tm_hour &&
            check.tm_min==value.tm_min && check.tm_sec==value.tm_sec,"RTC calendar does not round-trip as UTC");
    bounds(epoch);return epoch;
}
inline rtc_time calendar(int64_t epoch) {
    bounds(epoch);const time_t timestamp=epoch;std::tm value{};rtc_time result{};
    require(gmtime_r(&timestamp,&value),"Cannot encode RTC UTC");
    result.tm_year=value.tm_year;result.tm_mon=value.tm_mon;result.tm_mday=value.tm_mday;
    result.tm_hour=value.tm_hour;result.tm_min=value.tm_min;result.tm_sec=value.tm_sec;
    result.tm_wday=value.tm_wday;result.tm_yday=value.tm_yday;result.tm_isdst=0;
    require(utc(result)==epoch,"RTC UTC encoding differs");return result;
}
inline bool requested() {return fs::exists(fs::path(node)/"qcom,uefi-rtc-info");}

// The kernel driver owns the RTCInfo transaction. Userspace uses only RTC ioctls.
class Device {
    int fd_=-1;
public:
    explicit Device(bool writable) {
        require(requested(),"The running kernel has no secure RTC offset contract");
        require(!fs::exists(fs::path(node)/"allow-set-time") && !fs::exists(fs::path(node)/"nvmem-cells"),
                "Conflicting RTC direct-write or NVMEM contract");
        require(fs::canonical("/sys/class/rtc/rtc0/device/of_node")==fs::canonical(node) &&
                fs::canonical("/sys/class/rtc/rtc0/device/driver").filename()=="rtc-pm8xxx",
                "RTC0 is not the measured PMK secure RTC");
        std::ifstream identity("/sys/class/rtc/rtc0/dev");std::string text;unsigned major_number=0,minor_number=0;char tail=0;
        require(bool(std::getline(identity,text)) && text.size()<32 &&
                sscanf(text.c_str(),"%u:%u%c",&major_number,&minor_number,&tail)==2,"RTC device identity unavailable");
        int raw=-1;
        for(unsigned attempt=0;attempt<21;++attempt) {
            raw=open("/dev/rtc0",(writable?O_RDWR:O_RDONLY)|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK);
            if(raw>=0 || errno!=EBUSY || attempt==20)break;
            usleep(10000);
        }
        require(raw>=0,"Cannot open the admitted RTC device");
        struct stat st{};
        if(fstat(raw,&st) || !S_ISCHR(st.st_mode) || st.st_uid!=0 || (st.st_mode&S_IWOTH) ||
           st.st_rdev!=makedev(major_number,minor_number)) {
            close(raw);throw std::runtime_error("RTC character device changed");
        }
        fd_=raw;
    }
    ~Device(){if(fd_>=0)close(fd_);}
    Device(const Device&)=delete;Device& operator=(const Device&)=delete;
    std::optional<int64_t> read() const {
        rtc_time value{};
        if(ioctl(fd_,RTC_RD_TIME,&value)<0) {
            if(errno==ENODATA)return {};
            throw std::runtime_error("RTC read failed; no write attempted");
        }
        return utc(value);
    }
    void write(int64_t epoch) const {
        auto value=calendar(epoch);
        require(ioctl(fd_,RTC_SET_TIME,&value)==0,"Secure RTC write failed; persistence is not confirmed");
    }
};

template<class Read,class Write,class Clock>
Result synchronize(Read read,Write write,Clock clock) {
    const auto first=clock();
    if(!first.synchronized)return Result::deferred;
    bounds(first.utc);
    const auto previous=read();
    if(previous){bounds(*previous);if(distance(*previous,first.utc)<=1)return Result::correct;}
    const auto authorized=clock();bounds(authorized.utc);
    require(authorized.synchronized && authorized.utc>=first.utc && authorized.utc-first.utc<=2,
            "System time or synchronization changed before RTC write");
    write(authorized.utc);
    const auto after=clock();bounds(after.utc);
    const auto verified=read();
    require(after.synchronized && after.utc>=authorized.utc && after.utc-authorized.utc<=3 &&
            verified.has_value(),"RTC write could not be verified against synchronized UTC");
    bounds(*verified);
    require(distance(*verified,after.utc)<=2 && *verified>=authorized.utc-1,"RTC readback differs from synchronized UTC");
    return Result::written;
}
template<class Read,class Clock>
void check(Read read,Clock clock) {
    const auto sample=clock();bounds(sample.utc);const auto value=read();
    require(value.has_value(),"RTC calendar is uninitialized; trusted UTC provisioning is pending");
    bounds(*value);require(distance(*value,sample.utc)<=2,"RTC calendar differs from the system clock");
}
} // namespace uke_rtc
