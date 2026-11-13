// SPDX-License-Identifier: MIT
#include "motion-source.hpp"
#include "motion-ownership.hpp"
#include "motion-journal.hpp"
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <json/json.h>
#include <linux/fs.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace uke::motion {
namespace {
namespace fs=std::filesystem;
constexpr auto journal_path="/run/uke-control-motion/ownership.json";
std::uint64_t now() {
    timespec value{};if(clock_gettime(CLOCK_BOOTTIME,&value)<0)return 0;
    return std::uint64_t(value.tv_sec)*1000000+std::uint64_t(value.tv_nsec)/1000;
}
struct Fd {
    int value=-1;
    explicit Fd(int fd=-1):value(fd){}
    ~Fd(){if(value>=0)::close(value);}
    Fd(const Fd&)=delete;Fd& operator=(const Fd&)=delete;
};
std::optional<std::string> read_file(const fs::path& path,std::size_t limit=512) {
    Fd fd(::open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW));if(fd.value<0)return {};
    std::string text(limit+1,'\0');const auto count=::read(fd.value,text.data(),text.size());
    if(count<0||std::size_t(count)>limit)return {};
    text.resize(std::size_t(count));return std::string(trim(text));
}
bool write_file(const fs::path& path,std::string_view text) {
    Fd fd(::open(path.c_str(),O_WRONLY|O_CLOEXEC|O_NOFOLLOW));
    return fd.value>=0&&::write(fd.value,text.data(),text.size())==ssize_t(text.size());
}
struct Device final:Store {
    fs::path directory;
    Fd fd;
    std::uint64_t inode=0,device=0;
    unsigned device_major=0,device_minor=0;
    explicit Device(fs::path path):directory(std::move(path)){}
    bool open() {
        // /sys/bus contains class symlinks; pin the canonical kernel directory.
        std::error_code error;directory=fs::canonical(directory,error);
        if(error||!directory.string().starts_with("/sys/devices/"))return false;
        const auto basename=directory.filename().string();
        if(!basename.starts_with("iio:device")||!number<unsigned>(std::string_view(basename).substr(10)))return false;
        const auto dev=read_file(directory/"dev");if(!dev)return false;
        const auto colon=dev->find(':');if(colon==dev->npos)return false;
        const auto major_number=number<unsigned>(std::string_view(*dev).substr(0,colon));
        const auto minor_number=number<unsigned>(std::string_view(*dev).substr(colon+1));
        if(!major_number||!minor_number)return false;
        device_major=*major_number;device_minor=*minor_number;
        fd.value=::open((fs::path("/dev")/basename).c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW);
        struct stat node{},sysfs{};
        if(fd.value<0||::fstat(fd.value,&node)<0||!S_ISCHR(node.st_mode)||
           major(node.st_rdev)!=device_major||minor(node.st_rdev)!=device_minor||
           ::stat(directory.c_str(),&sysfs)<0)return false;
        inode=sysfs.st_ino;device=sysfs.st_dev;return true;
    }
    std::optional<std::string> read(std::string_view path)override {
        return setting_path(path)?read_file(directory/std::string(path)):std::optional<std::string>{};
    }
    bool write(std::string_view path,std::string_view text)override {
        return setting_path(path)&&write_file(directory/std::string(path),text);
    }
    bool save(const std::vector<Setting>& settings)override {
        Json::Value root(Json::objectValue);root["version"]=1;root["path"]=directory.string();
        root["inode"]=Json::UInt64(inode);root["device"]=Json::UInt64(device);
        root["major"]=device_major;root["minor"]=device_minor;root["settings"]=Json::arrayValue;
        for(const auto& s:settings){Json::Value row(Json::arrayValue);row.append(s.path);row.append(s.before);row.append(s.after);root["settings"].append(row);}
        Json::StreamWriterBuilder writer;writer["indentation"]="";const auto text=Json::writeString(writer,root);
        return Journal(journal_path).save(text);
    }
    bool clear()override {return Journal(journal_path).clear();}
};
bool recover() {
    Journal journal(journal_path);
    if(!journal.discard_pending())return false;
    const auto data=journal.load();
    if(data.status==Journal::Status::missing)return true;
    if(data.status!=Journal::Status::valid)return false;
    Json::CharReaderBuilder parser;parser["failIfExtra"]=true;parser["rejectDupKeys"]=true;
    std::unique_ptr<Json::CharReader> reader(parser.newCharReader());Json::Value root;std::string error;
    if(!reader->parse(data.text.data(),data.text.data()+data.text.size(),&root,&error)||!root.isObject()||
       root["version"]!=1||!root["path"].isString()||!root["inode"].isUInt64()||!root["device"].isUInt64()||
       !root["major"].isUInt()||!root["minor"].isUInt()||!root["settings"].isArray()||root["settings"].size()>11)return false;
    std::vector<Setting> settings;
    for(const auto& row:root["settings"]){
        if(!row.isArray()||row.size()!=3||!row[0].isString()||!row[1].isString()||!row[2].isString())return false;
        settings.push_back({row[0].asString(),row[1].asString(),row[2].asString()});
    }
    if(!settings_valid(settings))return false;
    Device device(root["path"].asString());
    if(!device.open()||device.inode!=root["inode"].asUInt64()||device.device!=root["device"].asUInt64()||
       device.device_major!=root["major"].asUInt()||device.device_minor!=root["minor"].asUInt())return false;
    return restore(device,settings);
}

class Iio final:public Source {
public:
    Iio(sd_event* event,std::function<void(Snapshot)> notify):m_event(event),m_notify(std::move(notify)){}
    ~Iio()override {stop();}
    bool start()override {
        if(m_device)return true;
        // Production admission remains closed until the kernel/DT owner proves
        // the fitted gravity frame, acquisition clock and runtime capability.
        // A sensor name, RIGID_BODY value or another device's matrix is not it.
        constexpr bool admitted=false;
        if(!admitted)return false;
        if(!recover())return false;
        std::error_code error;
        for(const auto& entry:fs::directory_iterator("/sys/bus/iio/devices",error)){
            if(error)break;
            auto device=std::make_unique<Device>(entry.path());
            const auto matrix=read_file(entry.path()/"in_gravity_mount_matrix");
            if(!matrix)continue;
            const auto mount=Mount::parse(*matrix);if(!mount||!device->open()||device->read("buffer/enable")!="0")continue;
            if(prepare(*device,*mount)){m_device=std::move(device);return true;}
            // A retained journal must never be replaced or another sensor used.
            if(::access(journal_path,F_OK)==0)return false;
        }
        return false;
    }
    void stop()override {
        m_io=sd_event_source_unref(m_io);m_deadline=sd_event_source_unref(m_deadline);
        if(m_device){restore(*m_device,m_settings);m_device.reset();}
        m_samples.begin({},0);m_settings.clear();
    }
    Snapshot current()const override {return m_samples.current(now());}
private:
    sd_event* m_event;
    std::function<void(Snapshot)> m_notify;
    sd_event_source *m_io=nullptr,*m_deadline=nullptr;
    std::unique_ptr<Device> m_device;
    Samples m_samples;
    Layout m_layout;
    Vector m_scale{};
    std::vector<Setting> m_settings;
    bool prepare(Device& device,const Mount& mount) {
        std::vector<Channel> channels;std::vector<Setting> settings;
        for(auto name:{"in_gravity_x","in_gravity_y","in_gravity_z","in_accel_linear_x","in_accel_linear_y","in_accel_linear_z","in_timestamp"}){
            const std::string prefix=std::string("scan_elements/")+name;
            const auto index=read_file(device.directory/(prefix+"_index")),type=read_file(device.directory/(prefix+"_type"));
            if(!index||!type)return false;
            const auto index_value=number<unsigned>(*index);const auto type_value=ScanType::parse(*type);
            if(!index_value||!type_value)return false;
            channels.push_back({name,*index_value,*type_value});
            const auto enabled=device.read(prefix+"_en");if(!enabled||(*enabled!="0"&&*enabled!="1"))return false;
            settings.push_back({prefix+"_en",*enabled,"1"});
        }
        const auto layout=Layout::make(channels);if(!layout)return false;
        // Reject an additional enabled channel rather than decode the wrong
        // scan stride or alter another consumer's selection.
        std::error_code error;
        for(const auto& entry:fs::directory_iterator(device.directory/"scan_elements",error)){
            if(error)return false;
            const auto filename=entry.path().filename().string();
            if(filename.ends_with("_en")&&!setting_path("scan_elements/"+filename))return false;
        }
        for(unsigned axis=0;axis<3;++axis){
            const auto scale=read_file(device.directory/(std::string("in_gravity_")+"xyz"[axis]+"_scale"));
            const auto common=scale?scale:read_file(device.directory/"in_gravity_scale");
            if(!common)return false;
            const auto value=number<double>(*common);if(!value||!std::isfinite(*value)||*value<=0)return false;m_scale[axis]=*value;
        }
        for(const auto& [path,value]:std::array<std::pair<const char*,const char*>,3>{{{"current_timestamp_clock","boottime"},{"buffer/length","4"},{"buffer/watermark","1"}}}){
            const auto old=device.read(path);if(!old)return false;settings.push_back({path,*old,value});
        }
        settings.push_back({"buffer/enable","0","1"});
        const auto epoch=now();if(!epoch||!configure(device,settings))return false;
        m_settings=settings;m_layout=*layout;m_samples.begin(mount,epoch);
        if(sd_event_add_io(m_event,&m_io,device.fd.value,EPOLLIN|EPOLLERR|EPOLLHUP,read_ready,this)<0){
            restore(device,settings);m_settings.clear();return false;
        }
        if(sd_event_add_time(m_event,&m_deadline,CLOCK_BOOTTIME,epoch+maximum_sample_age_usec,1000,expired,this)<0){
            m_io=sd_event_source_unref(m_io);restore(device,settings);m_settings.clear();return false;
        }
        return true;
    }
    void failed() {stop();m_notify({});}
    static int read_ready(sd_event_source*,int fd,std::uint32_t events,void* data) {
        auto& self=*static_cast<Iio*>(data);
        if(events&(EPOLLERR|EPOLLHUP)){self.failed();return 0;}
        std::array<std::byte,56*32> buffer{};
        const auto count=::read(fd,buffer.data(),buffer.size()/self.m_layout.bytes*self.m_layout.bytes);
        if(count<0&&(errno==EAGAIN||errno==EINTR))return 0;
        if(count<=0||std::size_t(count)%self.m_layout.bytes){self.failed();return 0;}
        for(std::size_t offset=0;offset<std::size_t(count);offset+=self.m_layout.bytes){
            const auto packet=self.m_layout.read(std::span(buffer).subspan(offset,self.m_layout.bytes),self.m_scale);
            if(!packet){self.failed();return 0;}
            const auto time=now();const bool changed=self.m_samples.observe(*packet,time);
            const auto snapshot=self.m_samples.current(time);
            if(changed)self.m_notify(snapshot);
            if(snapshot.known){
                sd_event_source_set_time(self.m_deadline,snapshot.timestamp_usec+maximum_sample_age_usec);
                sd_event_source_set_enabled(self.m_deadline,SD_EVENT_ONESHOT);
            }
        }
        return 0;
    }
    static int expired(sd_event_source*,std::uint64_t,void* data) {
        auto& self=*static_cast<Iio*>(data);
        if(self.m_samples.expire(now()))self.m_notify({});
        return 0;
    }
};
}
std::unique_ptr<Source> iio_source(sd_event* event,std::function<void(Snapshot)> notify) {return std::make_unique<Iio>(event,std::move(notify));}
bool recover_iio(){return recover();}
}
