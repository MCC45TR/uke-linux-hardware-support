// SPDX-License-Identifier: MIT
#include "sensors.hpp"
#include "cli.hpp"
#include "front-color.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <csignal>
#include <fstream>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <systemd/sd-bus.h>
#include <systemd/sd-json.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
namespace uke::sensors {
namespace fs=std::filesystem;
static std::string read(const fs::path& p) {
    std::ifstream f(p);std::string s;std::array<char,4097> b{};
    if(!f)return {};
    f.read(b.data(),b.size());if(f.gcount()>4096)return {};
    s.assign(b.data(),size_t(f.gcount()));while(!s.empty()&&(s.back()=='\n'||s.back()=='\r'))s.pop_back();return s;
}
static std::string clean(std::string s) {
    for(auto& c:s)if(static_cast<unsigned char>(c)<32||c==127)c='?';
    return s.substr(0,256);
}
static std::optional<double> number(const std::string& s) {
    double v{};auto r=std::from_chars(s.data(),s.data()+s.size(),v);
    if(s.empty()||r.ec!=std::errc{}||r.ptr!=s.data()+s.size()||!std::isfinite(v))return {};
    return v;
}
static std::vector<fs::path> entries(const fs::path& p) {
    std::vector<fs::path> result;std::error_code ec;
    fs::directory_iterator it(p,fs::directory_options::skip_permission_denied,ec),end;
    while(!ec&&it!=end&&result.size()<4096){result.push_back(it->path());it.increment(ec);}
    std::sort(result.begin(),result.end());return result;
}
static std::string iio_unit(const std::string& kind) {
    static const std::map<std::string,std::string> units{{"accel","m/s^2"},{"anglvel","rad/s"},{"magn","gauss"},{"illuminance","lux"},
        {"distance","m"},{"pressure","kPa"},{"temp","mC"},{"voltage","mV"},{"current","mA"},{"humidityrelative","m%"},{"steps","count"}};
    const auto it=units.find(kind);return it==units.end()?"unverified":it->second;
}
std::vector<Reading> sysfs_snapshot(const fs::path& sys) {
    std::vector<Reading> rows;
    const std::regex hw("^(temp|in|curr|power|fan|humidity|energy)[0-9]+_(input|average)$");
    const std::regex iio("^in_([a-z]+)[0-9]*(_[a-z0-9]+)*_(input|raw)$");
    for(const auto& group:{std::string("class/hwmon"),std::string("bus/iio/devices"),std::string("class/thermal"),std::string("class/backlight")}) {
        for(const auto& dir:entries(sys/group)) {
            const auto dirname=dir.filename().string();
            if(group=="bus/iio/devices"&&!dirname.starts_with("iio:device"))continue;
            if(group=="class/thermal"&&!dirname.starts_with("thermal_zone"))continue;
            auto device=read(dir/"name");if(device.empty())device=read(dir/"type");if(device.empty())device=dirname;
            for(const auto& file:entries(dir)) {
                if(rows.size()>=4096)throw std::runtime_error("Sensor inventory exceeds 4096 channels");
                const auto name=file.filename().string();std::smatch match;bool accepted=false;
                std::string unit;double divisor=1;std::optional<double> scale=1,offset=0;
                if(group=="class/hwmon"&&std::regex_match(name,match,hw)) {
                    accepted=true;const auto kind=match[1].str();
                    if(kind=="temp"){unit="C";divisor=1000;}else if(kind=="in"){unit="V";divisor=1000;}
                    else if(kind=="curr"){unit="A";divisor=1000;}else if(kind=="power"){unit="W";divisor=1000000;}
                    else if(kind=="humidity"){unit="%";divisor=1000;}else if(kind=="energy"){unit="J";divisor=1000000;}else unit="RPM";
                } else if(group=="bus/iio/devices"&&std::regex_match(name,match,iio)) {
                    accepted=true;unit=iio_unit(match[1]);
                    if(name.ends_with("_raw")) {
                        const auto stem=name.substr(0,name.size()-4),common="in_"+match[1].str();
                        auto s=read(dir/(stem+"_scale"));if(s.empty())s=read(dir/(common+"_scale"));scale=number(s);
                        auto o=read(dir/(stem+"_offset"));if(o.empty())o=read(dir/(common+"_offset"));if(!o.empty())offset=number(o);
                        if(!scale){scale=1;offset=0;unit="raw counts";}
                    }
                } else if(group=="class/thermal"&&name=="temp"){accepted=true;unit="C";divisor=1000;}
                else if(group=="class/backlight"&&(name=="actual_brightness"||name=="max_brightness"||name=="brightness")){accepted=true;unit="driver units";}
                if(!accepted)continue;
                auto value=number(read(file));if(value&&offset&&scale){*value=(*value+*offset)**scale/divisor;if(!std::isfinite(*value))value.reset();}else value.reset();
                auto label=read(dir/(name.substr(0,name.rfind('_'))+"_label"));
                rows.push_back({file.lexically_relative(sys).string(),clean(device)+" / "+clean(label.empty()?name:label),unit,value?"read (sample age unknown)":"unavailable",value,{}});
            }
        }
    }
    return rows;
}
static std::string quote(const std::string& s) {
    std::ostringstream out;out<<'"';
    for(unsigned char c:s){if(c=='"'||c=='\\')out<<'\\'<<c;else if(c<32)out<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<unsigned(c)<<std::dec;else out<<c;}
    out<<'"';return out.str();
}
std::string json(const std::vector<Reading>& rows) {
    std::ostringstream out;out<<"{\"schema\":\"org.uke.control.sensors.v1\",\"observed_monotonic_ns\":"<<std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()<<",\"readings\":[";
    bool first=true;for(const auto& r:rows){if(!first)out<<',';first=false;out<<"{\"source\":"<<quote(r.source)<<",\"label\":"<<quote(r.label)<<",\"unit\":"<<quote(r.unit)<<",\"status\":"<<quote(r.status)<<",\"value\":";if(r.value)out<<std::setprecision(17)<<*r.value;else out<<"null";out<<",\"text\":"<<(r.text.empty()?"null":quote(r.text))<<'}';}
    return out.str()+"]}\n";
}
class Proxy {
    sd_bus* bus=nullptr;
    std::string owner;
    struct Claim {const char* path;const char* iface;const char* name;bool held=false;};
    std::array<Claim,4> claims{{{"/net/hadess/SensorProxy","net.hadess.SensorProxy","Accelerometer",false},
        {"/net/hadess/SensorProxy","net.hadess.SensorProxy","Light",false},{"/net/hadess/SensorProxy","net.hadess.SensorProxy","Proximity",false},
        {"/net/hadess/SensorProxy/Compass","net.hadess.SensorProxy.Compass","Compass",false}}};
    using Message=std::unique_ptr<sd_bus_message,decltype(&sd_bus_message_unref)>;
    Message call(const Claim& c,const std::string& operation,const char* property=nullptr) {
        sd_bus_message *m=nullptr,*reply=nullptr;
        if(!bus||owner.empty()||sd_bus_message_new_method_call(bus,&m,owner.c_str(),c.path,property?"org.freedesktop.DBus.Properties":c.iface,operation.c_str())<0)return {nullptr,sd_bus_message_unref};
        Message request(m,sd_bus_message_unref);sd_bus_message_set_auto_start(m,0);
        if(property&&sd_bus_message_append(m,"ss",c.iface,property)<0)return {nullptr,sd_bus_message_unref};
        if(sd_bus_call(bus,m,500000,nullptr,&reply)<0)return {nullptr,sd_bus_message_unref};
        return {reply,sd_bus_message_unref};
    }
    Reading get(const Claim& c,const char* name,const std::string& unit) {
        Reading row{"SensorProxy/"+std::string(name),name,unit,"unavailable",{}, {}};
        auto reply=call(c,"Get",name);if(!reply||sd_bus_message_enter_container(reply.get(),'v',nullptr)<0)return row;
        char type=0;const char* contents=nullptr;
        if(sd_bus_message_peek_type(reply.get(),&type,&contents)<=0)return row;
        if(type=='d'){double value{};if(sd_bus_message_read(reply.get(),"d",&value)>0&&std::isfinite(value))row.value=value;}
        else if(type=='b'){int b{};if(sd_bus_message_read(reply.get(),"b",&b)>0)row.text=b?"true":"false";}
        else if(type=='s'){const char* s=nullptr;if(sd_bus_message_read(reply.get(),"s",&s)>0&&s)row.text=clean(s);}
        if(row.value||!row.text.empty())row.status="cached (sample age unknown)";
        return row;
    }
public:
    explicit Proxy(bool claim) {
        if(sd_bus_open_system(&bus)<0)return;
        sd_bus_message* response=nullptr;sd_bus_set_method_call_timeout(bus,500000);
        if(sd_bus_call_method(bus,"org.freedesktop.DBus","/org/freedesktop/DBus","org.freedesktop.DBus","GetNameOwner",nullptr,&response,"s","net.hadess.SensorProxy")>=0) {
            Message reply(response,sd_bus_message_unref);const char* s=nullptr;
            if(sd_bus_message_read(reply.get(),"s",&s)>0&&s)owner=s;
        }
        if(!claim)return;
        for(auto& c:claims) {
            const auto capability=std::string("Has")+(std::string(c.name)=="Light"?"AmbientLight":c.name);
            if(get(c,capability.c_str(),"").text=="true")c.held=bool(call(c,"Claim"+std::string(c.name)));
        }
    }
    ~Proxy(){for(const auto& c:claims)if(c.held)call(c,"Release"+std::string(c.name));sd_bus_flush_close_unref(bus);}
    void append(std::vector<Reading>& rows,bool list) {
        for(const auto& c:claims) {
            const auto capability=std::string("Has")+(std::string(c.name)=="Light"?"AmbientLight":c.name);
            auto present=get(c,capability.c_str(),"");const bool available=present.text=="true";rows.push_back(present);
            if(list)continue;
            auto add=[&](const char* name,const std::string& unit){auto row=get(c,name,unit);if(!c.held||!available){row.value.reset();row.text.clear();row.status="unavailable (sensor absent or not claimed)";}rows.push_back(row);};
            if(std::string(c.name)=="Accelerometer"){add("AccelerometerOrientation","");add("AccelerometerTilt","");}
            if(std::string(c.name)=="Light"){auto unit=get(c,"LightLevelUnit","");add("LightLevel",unit.text.empty()?"unverified":unit.text);}
            if(std::string(c.name)=="Proximity")add("ProximityNear","near/far; not distance");
            if(std::string(c.name)=="Compass")add("CompassHeading","degrees");
        }
    }
};
static void color(std::vector<Reading>& rows) {
    const auto text=display::front_color_status();sd_json_variant* doc=nullptr;
    if(sd_json_parse(text.c_str(),static_cast<sd_json_parse_flags_t>(0),&doc,nullptr,nullptr)<0)return;
    std::unique_ptr<sd_json_variant,decltype(&sd_json_variant_unref)> owned(doc,sd_json_variant_unref);
    auto available=sd_json_variant_by_key(doc,"available");
    const bool valid=sd_json_variant_is_boolean(available)&&sd_json_variant_boolean(available);
    auto reason=sd_json_variant_by_key(doc,"reader_reason");const std::string status=valid?"fresh snapshot; units unverified":sd_json_variant_is_string(reason)?sd_json_variant_string(reason):"unavailable";
    auto values=sd_json_variant_by_key(doc,"values");
    if(!valid||!sd_json_variant_is_array(values)||!sd_json_variant_elements(values)){rows.push_back({"front-color","Front color","unverified",status,{}, {}});return;}
    for(size_t i=0;i<sd_json_variant_elements(values);i++){auto v=sd_json_variant_by_index(values,i);if(sd_json_variant_is_real(v))rows.push_back({"front-color/"+std::to_string(i),"Front color field "+std::to_string(i),"unverified",status,sd_json_variant_real(v),{}});}
}
static void switches(std::vector<Reading>& rows) {
    for(const auto& node:entries("/sys/class/input")) {
        const auto name=node.filename().string();if(!name.starts_with("event"))continue;
        const auto bits=read(node/"device/capabilities/sw");unsigned long supported=0;
        auto parsed=std::from_chars(bits.data(),bits.data()+bits.size(),supported,16);
        if(parsed.ec!=std::errc{}||parsed.ptr!=bits.data()+bits.size()||!supported)continue;
        const int fd=open(("/dev/input/"+name).c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW);
        unsigned long state=0;const bool ok=fd>=0&&ioctl(fd,EVIOCGSW(sizeof(state)),&state)>=0;
        if(fd>=0)close(fd);
        for(unsigned i=0;i<=SW_MAX;i++)if(supported&(1UL<<i)) {
            const auto label=i==SW_LID?"Lid closed":i==SW_TABLET_MODE?"Tablet mode":i==SW_DOCK?"Dock present":"Input switch "+std::to_string(i);
            rows.push_back({"input/"+name+"/switch"+std::to_string(i),label,"boolean",ok?"kernel switch state":"unavailable (input permission or disconnected)",ok?std::optional<double>(bool(state&(1UL<<i))):std::nullopt,{}});
        }
    }
}
static volatile sig_atomic_t stopped=0;
static void stop(int){stopped=1;}
static unsigned positive(const std::string& s,unsigned max) {
    unsigned v{};auto r=std::from_chars(s.data(),s.data()+s.size(),v);
    if(s.empty()||r.ec!=std::errc{}||r.ptr!=s.data()+s.size()||v<1||v>max)throw std::runtime_error("Expected integer in 1.."+std::to_string(max));
    return v;
}
int command(int argc,char** argv,bool as_json) {
    if(argc && std::string_view(argv[0])=="capture") {
        if(argc<2 || argc>3)throw std::runtime_error("Usage: sensors capture DATATYPE [SECONDS] [--json]");
        if(argc==3)(void)positive(argv[2],60);
        std::vector<char*> args{const_cast<char*>("/usr/libexec/uke-control/ssc-sample"),argv[1]};
        if(argc==3)args.push_back(argv[2]);
        if(as_json)args.push_back(const_cast<char*>("--json"));
        args.push_back(nullptr);alarm(85);
        execv(args[0],args.data());throw std::runtime_error("Native sensor capture unavailable");
    }
    if(argc==1&&std::string_view(argv[0])=="--help") {
        std::cout<<"uke-control sensors [-nSECONDS | -n SECONDS] [--count=N] [--once] [--json] [--list]\n"
            "uke-control sensors capture DATATYPE [SECONDS] [--json]\n"
            "Default: one snapshot. -n5: repeat every five seconds; Ctrl-C stops. No device configuration is changed.\n";return 0;
    }
    unsigned interval=0,count=0;bool list=false,once=false;
    for(int i=0;i<argc;i++) {
        const std::string a=argv[i];
        if(a=="--json")as_json=true;
        else if(a=="--list"||a=="list")list=true;
        else if(a=="--once")once=true;
        else if(a.starts_with("--count="))count=positive(a.substr(8),1000000);
        else if(a=="-n"||a=="--interval"){if(++i>=argc)throw std::runtime_error("Missing sensor interval");if(interval)throw std::runtime_error("Duplicate sensor interval");interval=positive(argv[i],86400);}
        else if(a.starts_with("-n")){if(interval)throw std::runtime_error("Duplicate sensor interval");interval=positive(a.substr(2),86400);}
        else throw std::runtime_error("Unknown sensors option: "+a);
    }
    if((count>1&&!interval)||(once&&count>1))throw std::runtime_error("Repeated samples require -n; --once conflicts with --count > 1");
    if(once||list||!interval)count=1;
    Proxy proxy(!list);stopped=0;auto old_int=std::signal(SIGINT,stop),old_term=std::signal(SIGTERM,stop);
    struct Restore {decltype(old_int) a,b;~Restore(){std::signal(SIGINT,a);std::signal(SIGTERM,b);}} restore{old_int,old_term};
    auto next=std::chrono::steady_clock::now();unsigned samples=0;
    while(!stopped) {
        auto rows=sysfs_snapshot();proxy.append(rows,list);color(rows);switches(rows);
        if(as_json)std::cout<<json(rows);
        else {
            if(interval&&isatty(STDOUT_FILENO))std::cout<<"\033[H\033[2J";
            std::cout<<"Uke sensors | sample "<<samples+1<<" | "<<rows.size()<<" channels | Ctrl-C stops\n"
                "Polling time is not sensor acquisition time. Missing exports remain unavailable.\n";
            for(const auto& r:rows){std::cout<<r.label<<": ";if(r.value)std::cout<<std::setprecision(9)<<*r.value<<' '<<r.unit;else if(!r.text.empty())std::cout<<r.text;else std::cout<<"unavailable";std::cout<<" | "<<r.status<<" | "<<r.source<<'\n';}
        }
        std::cout.flush();if(!std::cout)break;
        ++samples;if(count&&samples>=count)break;
        next+=std::chrono::seconds(interval);
        if(next<std::chrono::steady_clock::now())next=std::chrono::steady_clock::now()+std::chrono::seconds(interval);
        while(!stopped&&std::chrono::steady_clock::now()<next)std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}
}
