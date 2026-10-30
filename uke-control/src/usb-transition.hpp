// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace uke::usb {
class TransitionInhibit {
    struct Fd {int value=-1;~Fd(){if(value>=0)close(value);}} fd_;
    std::filesystem::path path_;
    bool clear_=true;
    static void require(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
    static std::string text(const char* path) {
        std::ifstream input(path);std::string value;std::getline(input,value);
        require(bool(input)&&value.size()<4096,"USB_OWNER_IDENTITY_UNAVAILABLE");return value;
    }
    static std::string boot(){return text("/proc/sys/kernel/random/boot_id");}
    static std::string record(const char* phase) {
        const auto stat=text("/proc/self/stat");const auto end=stat.rfind(')');require(end!=stat.npos,"USB_OWNER_IDENTITY_UNAVAILABLE");
        std::istringstream fields(stat.substr(end+2));std::string field,start;
        for(unsigned index=3;index<=22;++index){require(bool(fields>>field),"USB_OWNER_IDENTITY_UNAVAILABLE");if(index==22)start=field;}
        return "format=1\nphase="+std::string(phase)+"\npid="+std::to_string(getpid())+"\nstart="+start+"\nboot="+boot()+"\n";
    }
    static std::map<std::string,std::string> decode(int fd) {
        require(lseek(fd,0,SEEK_SET)==0,"USB_TRANSITION_RECORD_UNREADABLE");
        std::array<char,513> data{};ssize_t n;do{n=::read(fd,data.data(),data.size());}while(n<0&&errno==EINTR);
        require(n>0&&n<=512,"USB_TRANSITION_RECOVERY_REQUIRED_LEGACY_OR_INVALID_RECORD");
        std::map<std::string,std::string> result;std::istringstream lines(std::string(data.data(),size_t(n)));std::string line;
        while(std::getline(lines,line)) {const auto at=line.find('=');require(at!=line.npos&&result.emplace(line.substr(0,at),line.substr(at+1)).second,"USB_TRANSITION_RECORD_INVALID");}
        require(result.size()==5&&result["format"]=="1"&&(result["phase"]=="reserved"||result["phase"]=="changing")&&
            !result["pid"].empty()&&result["pid"].find_first_not_of("0123456789")==std::string::npos&&
            !result["start"].empty()&&result["start"].find_first_not_of("0123456789")==std::string::npos&&
            result["boot"]==boot(),"USB_TRANSITION_RECOVERY_REQUIRED_UNKNOWN_OWNER");
        return result;
    }
    static void trusted(int fd) {
        struct stat st{};require(fd>=0&&!fstat(fd,&st)&&S_ISREG(st.st_mode)&&st.st_uid==0&&st.st_nlink==1&&(st.st_mode&0777)==0600,"USB_TRANSITION_RECORD_UNTRUSTED");
    }
    static void remove_own(int fd,const std::filesystem::path& path) {
        struct stat own{},live{};
        require(!fstat(fd,&own)&&!lstat(path.c_str(),&live)&&own.st_dev==live.st_dev&&own.st_ino==live.st_ino,"USB_TRANSITION_OWNER_CHANGED");
        require(!unlink(path.c_str()),"USB_TRANSITION_RECORD_CLEANUP_FAILED");
    }
    void save(const char* phase) {
        const auto value=record(phase);require(!ftruncate(fd_.value,0)&&lseek(fd_.value,0,SEEK_SET)==0,"USB_TRANSITION_RECORD_WRITE_FAILED");
        size_t offset=0;while(offset<value.size()){const auto n=write(fd_.value,value.data()+offset,value.size()-offset);if(n<0&&errno==EINTR)continue;require(n>0,"USB_TRANSITION_RECORD_WRITE_FAILED");offset+=size_t(n);}
        require(!fsync(fd_.value),"USB_TRANSITION_RECORD_SYNC_FAILED");
    }
public:
    static constexpr auto default_path="/run/senemos-uke/usb-gadget-inhibit";
    explicit TransitionInhibit(const std::filesystem::path& path=default_path):path_(path) {
        // Direct helpers retain the same open lock description after client loss.
        fd_.value=open(path.c_str(),O_CREAT|O_EXCL|O_RDWR|O_NOFOLLOW|O_NONBLOCK,0600);
        const bool fresh=fd_.value>=0;
        if(!fresh){require(errno==EEXIST,"USB_TRANSITION_RECORD_UNAVAILABLE");fd_.value=open(path.c_str(),O_RDWR|O_NOFOLLOW|O_NONBLOCK);}
        trusted(fd_.value);require(!flock(fd_.value,LOCK_EX|LOCK_NB),"USB_TRANSITION_ALREADY_INHIBITED");
        if(!fresh)require(decode(fd_.value)["phase"]=="reserved","USB_TRANSITION_RECOVERY_REQUIRED");
        save("reserved");
    }
    TransitionInhibit(const TransitionInhibit&)=delete;
    ~TransitionInhibit(){if(clear_&&fd_.value>=0)try{remove_own(fd_.value,path_);}catch(const std::exception&){};}
    void begin_write(){clear_=false;save("changing");}
    void complete(){clear_=true;}
    static std::string status(const std::filesystem::path& path=default_path) {
        Fd fd{open(path.c_str(),O_RDWR|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC)};
        if(fd.value<0){require(errno==ENOENT,"USB_TRANSITION_RECORD_UNAVAILABLE");return "absent";}
        trusted(fd.value);
        if(flock(fd.value,LOCK_EX|LOCK_NB))return "active";
        try{return decode(fd.value)["phase"]=="reserved"?"reserved_stale":"needs_recovery";}
        catch(const std::exception&){return "legacy_or_invalid_needs_review";}
    }
    static void recover(const std::function<bool()>& stable,const std::filesystem::path& path=default_path) {
        Fd fd{open(path.c_str(),O_RDWR|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC)};trusted(fd.value);
        require(!flock(fd.value,LOCK_EX|LOCK_NB),"USB_TRANSITION_STILL_ACTIVE");
        (void)decode(fd.value);require(stable(),"USB_TRANSITION_STATE_NOT_STABLE");remove_own(fd.value,path);
    }
};
}
