// SPDX-License-Identifier: MIT
#pragma once
// Private Linux UTC anchor for the read-only Uke PMIC counter. No PMIC writes.
#include <array>
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <fcntl.h>
#include <linux/magic.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>

namespace uke_counter_clock {
inline void need(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
inline constexpr int64_t first_utc=1577836800, last_utc=4102444800;
inline constexpr uint64_t counter_max=UINT32_MAX;
inline constexpr const char *record_name="rtc-counter-offset";
struct Record {uint64_t counter;int64_t utc;bool operator==(const Record &) const=default;};
struct Sample {int64_t utc;uint64_t boot_ns;bool synchronized;};
enum class Result {deferred,unchanged,saved,restored};
struct InvalidRecord:std::runtime_error {using std::runtime_error::runtime_error;};
inline void bounds(int64_t value) {need(value>=first_utc && value<last_utc,"Counter clock UTC outside supported bounds");}
inline uint64_t gap(int64_t a,int64_t b) {return a>=b?uint64_t(a-b):uint64_t(b-a);}
inline void valid(const Record &r) {need(r.counter<=counter_max,"Invalid RTC counter anchor");bounds(r.utc);}
inline int64_t mapped(const Record &r,uint64_t counter) {
    valid(r);need(counter<=counter_max && counter>=r.counter,"RTC counter reset or wrapped; trusted reprovisioning required");
    const auto delta=counter-r.counter;
    need(delta<uint64_t(last_utc-r.utc),"Counter clock UTC exceeds supported range");
    return r.utc+int64_t(delta);
}
inline uint32_t crc32(const unsigned char *data,size_t size) {
    uint32_t result=UINT32_MAX;
    for(size_t i=0;i<size;++i) {
        result^=data[i];
        for(unsigned bit=0;bit<8;++bit)result=(result>>1)^(0xedb88320U & (0U-(result&1)));
    }
    return ~result;
}
using Bytes=std::array<unsigned char,32>;
inline constexpr std::array<unsigned char,8> magic{'U','K','E','R','T','C',1,0};
inline void put(Bytes &data,size_t at,uint64_t value,unsigned size) {
    for(unsigned i=0;i<size;++i)data[at+i]=static_cast<unsigned char>(value>>(8*i));
}
inline uint64_t get(const Bytes &data,size_t at,unsigned size) {
    uint64_t value=0;for(unsigned i=0;i<size;++i)value|=uint64_t(data[at+i])<<(8*i);return value;
}
inline Bytes encode(const Record &r) {
    valid(r);Bytes data{};std::copy(magic.begin(),magic.end(),data.begin());
    put(data,8,r.counter,8);put(data,16,uint64_t(r.utc),8);put(data,24,crc32(data.data(),24),4);return data;
}
inline Record decode(const Bytes &data) {
    if(!std::equal(magic.begin(),magic.end(),data.begin()) || get(data,28,4)!=0 ||
       get(data,24,4)!=crc32(data.data(),24))throw InvalidRecord("Invalid private counter clock record format/checksum");
    const auto utc=get(data,16,8), counter=get(data,8,8);
    if(utc<uint64_t(first_utc) || utc>=uint64_t(last_utc) || counter>counter_max)
        throw InvalidRecord("Invalid private counter clock anchor bounds");
    return {counter,int64_t(utc)};
}
struct Fd {
    int value;
    explicit Fd(int fd):value(fd){need(fd>=0,"Cannot open counter clock state");}
    ~Fd(){if(value>=0)close(value);}
    Fd(const Fd &)=delete;Fd &operator=(const Fd &)=delete;
};
inline void trusted_dir(int fd,uid_t owner) {
    struct stat st{};need(fstat(fd,&st)==0 && S_ISDIR(st.st_mode) && st.st_uid==owner && !(st.st_mode&0022),
                        "Untrusted counter clock state directory");
}
inline void trusted_record(int fd,uid_t owner,bool full_size) {
    struct stat st{};
    need(fstat(fd,&st)==0 && S_ISREG(st.st_mode) && st.st_uid==owner && st.st_nlink==1 &&
         (st.st_mode&07777)==0600,"Untrusted private counter clock record");
    if(full_size && st.st_size!=32)throw InvalidRecord("Private counter clock record has an invalid size");
}
inline std::optional<Record> read_record(int dir,uid_t owner=0) {
    trusted_dir(dir,owner);
    const int raw=openat(dir,record_name,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
    if(raw<0 && errno==ENOENT)return {};
    Fd fd(raw);trusted_record(fd.value,owner,true);Bytes data{};size_t done=0;
    while(done<data.size()) {
        const auto n=read(fd.value,data.data()+done,data.size()-done);
        if(n<0 && errno==EINTR)continue;
        need(n>0,"Cannot read private counter clock record");done+=size_t(n);
    }
    unsigned char extra{};need(read(fd.value,&extra,1)==0,"Counter clock record changed while reading");
    return decode(data);
}
inline std::optional<Record> read_for_sync(int dir,uid_t owner=0) {
    try {return read_record(dir,owner);}catch(const InvalidRecord &){return {};}
}
inline void write_record(int dir,const Record &record,uid_t owner=0) {
    trusted_dir(dir,owner);const auto data=encode(record);
    // A synchronized clock may repair data corruption, never foreign metadata.
    const int old=openat(dir,record_name,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
    if(old>=0) {Fd input(old);trusted_record(input.value,owner,false);}
    else need(errno==ENOENT,"Cannot validate existing counter clock state");
    uint64_t random{};need(getrandom(&random,sizeof(random),0)==ssize_t(sizeof(random)),"Counter clock transaction entropy unavailable");
    const auto temporary=".rtc-counter-offset."+std::to_string(getpid())+"."+std::to_string(random);
    Fd fd(openat(dir,temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));
    bool renamed=false;
    try {
        trusted_record(fd.value,owner,false);size_t done=0;
        while(done<data.size()) {
            const auto n=write(fd.value,data.data()+done,data.size()-done);
            if(n<0 && errno==EINTR)continue;
            need(n>0,"Cannot write private counter clock record");done+=size_t(n);
        }
        need(fsync(fd.value)==0,"Cannot synchronize counter clock record");
        need(renameat(dir,temporary.c_str(),dir,record_name)==0,"Cannot commit counter clock record");renamed=true;
        need(fsync(dir)==0,"Cannot synchronize counter clock state directory");
        const auto verified=read_record(dir,owner);
        need(verified && *verified==record,"Committed counter clock record differs");
    } catch(...) {if(!renamed)(void)unlinkat(dir,temporary.c_str(),0);throw;}
}
inline void pair_ok(const Sample &a,const Sample &b,bool synchronized) {
    bounds(a.utc);bounds(b.utc);
    need(b.boot_ns>=a.boot_ns && b.boot_ns-a.boot_ns<=1000000000 && b.utc>=a.utc && b.utc-a.utc<=2,
         "System clock changed during RTC counter sampling");
    if(synchronized)need(a.synchronized && b.synchronized,"Trusted UTC changed during counter clock provisioning");
}
template<class ReadCounter,class Pause>
uint64_t advancing(ReadCounter read_counter,Pause pause) {
    const auto first=read_counter();need(first<=counter_max,"Invalid PMIC counter");
    for(unsigned i=0;i<6;++i) {
        pause();const auto next=read_counter();
        need(next<=counter_max && next>=first && next-first<=3,"PMIC counter changed discontinuously");
        if(next>first)return next;
    }
    throw std::runtime_error("PMIC counter did not advance; no clock change authorized");
}
template<class ReadCounter,class Clock,class Load,class Save,class Pause>
Result synchronize(ReadCounter read_counter,Clock clock,Load load,Save save,Pause pause) {
    if(!clock().synchronized)return Result::deferred;
    (void)advancing(read_counter,pause);
    const auto before=clock();const auto counter=read_counter();const auto after=clock();pair_ok(before,after,true);
    Record next{counter,after.utc};valid(next);
    const auto existing=load();
    if(existing) {
        std::optional<int64_t> previous;
        try {previous=mapped(*existing,counter);}catch(const std::runtime_error &){/* Fresh trusted UTC replaces stale mapping. */}
        if(previous && gap(*previous,after.utc)<=1)return Result::unchanged;
    }
    const auto authorized=clock();pair_ok(after,authorized,true);
    const auto fresh_counter=read_counter();
    need(gap(mapped(next,fresh_counter),authorized.utc)<=2,"Counter clock sample differs before persistence");
    save(next);
    const auto verified=load();const auto final=clock();pair_ok(authorized,final,true);
    need(verified && *verified==next && gap(mapped(*verified,read_counter()),final.utc)<=2,
         "Counter clock persistence could not be verified against trusted UTC");
    return Result::saved;
}
template<class ReadCounter,class Clock,class SetTime,class Pause>
Result restore(const Record &record,ReadCounter read_counter,Clock clock,SetTime set_time,Pause pause) {
    const auto first=clock();
    if(first.synchronized)return Result::deferred;
    (void)advancing(read_counter,pause);
    const auto before=clock();
    if(before.synchronized)return Result::deferred;
    need(before.utc>=0 && before.utc<last_utc,"Invalid startup system clock");
    need(before.boot_ns>=first.boot_ns && before.boot_ns-first.boot_ns<=3000000000,
         "Counter clock restore took too long");
    const auto target=mapped(record,read_counter());
    // Preserve the existing startup clock floor: never move wall time backwards.
    if(before.utc>=target-1)return Result::unchanged;
    const auto authorized=clock();
    if(authorized.synchronized)return Result::deferred;
    need(authorized.utc>=0 && authorized.utc<last_utc,"Invalid system clock before counter restore");
    need(authorized.boot_ns>=before.boot_ns && authorized.boot_ns-before.boot_ns<=1000000000 &&
         authorized.utc>=before.utc && authorized.utc-before.utc<=2,"System clock changed before counter restore");
    const auto fresh=mapped(record,read_counter());
    if(authorized.utc>=fresh-1)return Result::unchanged;
    need(set_time(fresh)==0,"Cannot restore UTC from the persistent PMIC counter anchor");
    const auto after=clock();bounds(after.utc);
    need(after.boot_ns>=authorized.boot_ns && after.boot_ns-authorized.boot_ns<=2000000000 &&
         gap(mapped(record,read_counter()),after.utc)<=2,"Restored counter clock differs from readback");
    return Result::restored;
}

// The exact running kernel's raw RTC class is the only hardware interface.
// This adapter never opens a block device, RTC character device or OEM persist.
class Counter {
    Fd directory_;
    static constexpr const char *node="/sys/firmware/devicetree/base/soc/qcom,spmi@c42d000/qcom,pmk8550@0/rtc@6100";
    static bool identity() {
        namespace fs=std::filesystem;std::error_code error;
        for(const auto *property:{"qcom,uefi-rtc-info","allow-set-time","nvmem-cells","nvmem-cell-names"})
            if(fs::exists(fs::path(node)/property,error) || error)return false;
        const auto actual=fs::canonical("/sys/class/rtc/rtc0/device/of_node",error);if(error)return false;
        const auto expected=fs::canonical(node,error);if(error || actual!=expected)return false;
        const auto driver=fs::canonical("/sys/class/rtc/rtc0/device/driver",error);
        return !error && driver.filename()=="rtc-pm8xxx";
    }
    static int open_directory() {
        need(identity(),"Measured raw PMK RTC backend is unavailable");
        const int raw=open("/sys/class/rtc/rtc0",O_RDONLY|O_DIRECTORY|O_CLOEXEC);
        if(raw<0)throw std::runtime_error("Cannot open the raw RTC class");
        struct statfs st{};
        if(fstatfs(raw,&st) || st.f_type!=SYSFS_MAGIC || !identity()) {
            close(raw);throw std::runtime_error("Raw RTC sysfs identity changed");
        }
        return raw;
    }
public:
    static bool available(){return identity();}
    Counter():directory_(open_directory()){}
    uint64_t read() const {
        Fd fd(openat(directory_.value,"since_epoch",O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC));
        std::array<char,32> text{};const auto n=::read(fd.value,text.data(),text.size());
        need(n>1 && n<int(text.size()) && text[size_t(n)-1]=='\n',"Malformed raw RTC counter");
        uint64_t value{};const auto parsed=std::from_chars(text.data(),text.data()+n-1,value);
        need(parsed.ec==std::errc{} && parsed.ptr==text.data()+n-1 && value<=counter_max,"Invalid raw RTC counter range");
        return value;
    }
};
} // namespace uke_counter_clock
