// SPDX-License-Identifier: MIT
#include "protocol.hpp"
#include <linux/magic.h>
#include <linux/tee.h>
#include <poll.h>
#include <sys/vfs.h>
#include <systemd/sd-daemon.h>
#include <thread>
namespace uke_pogo_auth {
static volatile sig_atomic_t stopping=0;
static void stop(int){stopping=1;}
static std::string_view read_kernel(int fd,Secret<256> &buffer) {
    struct statfs filesystem{};
    need(!fstatfs(fd,&filesystem)&&filesystem.f_type==SYSFS_MAGIC,"Kernel-owned sysfs descriptor required");
    need(lseek(fd,0,SEEK_SET)==0,"Cannot reset kernel attribute read");
    ssize_t size;
    do{size=read(fd,buffer.bytes.data(),buffer.bytes.size());}while(size<0&&errno==EINTR&&!stopping);
    if(size<0&&errno==ENODATA)return {};
    need(size>0&&size<ssize_t(buffer.bytes.size()),"Cannot read bounded kernel attribute");
    return {reinterpret_cast<const char*>(buffer.bytes.data()),size_t(size)};
}
static std::string_view property(const fs::path &path,Secret<256> &buffer) {
    Fd fd(open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW));return read_kernel(fd.value,buffer);
}
static bool compatible(std::string_view bytes,std::string_view name) {
    size_t p=0;
    while(p<bytes.size()) {
        const auto end=bytes.find('\0',p);need(end!=std::string_view::npos,"Unterminated board property");
        if(bytes.substr(p,end-p)==name)return true;p=end+1;
    }
    return false;
}
static void board() {
    Secret<256> buffer;const auto identity=property("/sys/firmware/devicetree/base/compatible",buffer);
    need(compatible(identity,"xiaomi,uke")&&compatible(identity,"qcom,sm7675"),"Exact Uke/SM7675 identity required");
}
static void release_name(const std::string &release) {
    need(token(release)&&release.starts_with("7.3.1-")&&release.ends_with("-unstable"),"Authentication candidates require an -unstable kernel ABI");
}
static Bytes firmware(const std::string &release) {
    release_name(release);
    auto image=file(fs::path("/usr/lib/firmware")/release/"qcom/uke/pogo-auth/devauth.mbn");
    need(uke_local_fw::admitted_auth(image),"Authentication image is not the exact reconstructed Uke TA");return image;
}
static void tee_identity() {
    Fd fd(open("/dev/tee0",O_RDWR|O_CLOEXEC|O_NOFOLLOW));struct stat st{};tee_ioctl_version_data version{};
    need(!fstat(fd.value,&st)&&S_ISCHR(st.st_mode)&&st.st_uid==0&&!(st.st_mode&0022),"Protected TEE character device required");
    need(!ioctl(fd.value,TEE_IOC_VERSION,&version)&&version.impl_id==TEE_IMPL_ID_QTEE&&
         (version.gen_caps&TEE_GEN_CAP_OBJREF),"The selected TEE is not QTEE with object-reference support");
}
class Bridge {
    Fd directory_,request_;
    static fs::path discover() {
        const fs::path expected="/sys/firmware/devicetree/base/soc/qcom,qupv3_0_geni_se@ac0000/i2c@a8c000/nanosic@4c";
        std::optional<fs::path> found;
        for(const auto &entry:fs::directory_iterator("/sys/bus/i2c/devices")) {
            std::error_code error;const auto node=fs::canonical(entry.path()/"of_node",error);
            if(error||node!=expected)continue;
            const auto driver=fs::canonical(entry.path()/"driver",error);
            if(error||driver.filename()!="hid-nanosic-uke")continue;
            Secret<256> buffer;const auto names=property(entry.path()/"of_node/compatible",buffer);
            need(compatible(names,"xiaomi,uke-wn8030"),"Unexpected pogo compatible");
            need(!found,"Multiple pogo authentication bridges are not admitted");found=fs::canonical(entry.path());
        }
        need(bool(found),"No qualified Uke pogo kernel authentication bridge is bound");return *found;
    }
public:
    Bridge():directory_(open(discover().c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW)),
             request_(openat(directory_.value,"auth_request",O_RDONLY|O_CLOEXEC|O_NOFOLLOW)){}
    std::optional<Request> current() {
        Secret<256> buffer;const auto data=read_kernel(request_.value,buffer);
        if(data.empty())return std::nullopt;return parse(data);
    }
    void wait() {
        pollfd event{request_.value,POLLPRI|POLLERR,0};const auto status=poll(&event,1,1000);
        need(status>=0||errno==EINTR,"Cannot poll the pogo authentication bridge");
        if(status>0)need(event.revents&POLLPRI,"Pogo authentication bridge was removed");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    bool submit(uint32_t epoch,const Secret<16> &token) {
        Fd out(openat(directory_.value,"auth_token",O_WRONLY|O_CLOEXEC|O_NOFOLLOW));struct statfs filesystem{};
        need(!fstatfs(out.value,&filesystem)&&filesystem.f_type==SYSFS_MAGIC,"Kernel-owned token attribute required");
        Secret<48> buffer;const auto prefix=snprintf(reinterpret_cast<char*>(buffer.bytes.data()),buffer.bytes.size(),"%u ",epoch);
        need(prefix>0&&prefix<=11,"Invalid authentication epoch encoding");constexpr char digits[]="0123456789abcdef";
        for(size_t i=0;i<16;i++){buffer.bytes[prefix+2*i]=digits[token.bytes[i]>>4];buffer.bytes[prefix+2*i+1]=digits[token.bytes[i]&15];}
        const size_t size=prefix+33;buffer.bytes[size-1]='\n';ssize_t result;
        do{result=write(out.value,buffer.bytes.data(),size);}while(result<0&&errno==EINTR&&!stopping);
        if(result<0&&errno==ESTALE)return false;
        need(result==ssize_t(size),"Kernel refused the current authentication token");
        return true;
    }
};
static int run(int argc,char **argv) {
    need(argc>=2,"Usage: uke-pogo-auth --check [KERNEL_RELEASE] | --serve | --once");board();
    utsname current{};need(!uname(&current),"Cannot identify the running kernel");
    const std::string mode=argv[1];
    if(mode=="--check") {
        need(argc==2||argc==3,"Invalid firmware check arguments");
        const auto image=firmware(argc==3?argv[2]:current.release);
        std::cout<<"Exact versioned Uke authentication image verified; no TEE object was opened\n";return 0;
    }
    need(argc==2&&(mode=="--serve"||mode=="--once"),"Unknown authentication operation");
    need(geteuid()==0&&getuid()==0,"Administrator authentication is required");release_name(current.release);
    Bridge bridge;
    if(mode=="--once")need(bool(bridge.current()),"No pending kernel authentication challenge");
    struct sigaction action{};action.sa_handler=stop;sigemptyset(&action.sa_mask);
    need(!sigaction(SIGTERM,&action,nullptr)&&!sigaction(SIGINT,&action,nullptr),"Cannot install stop handlers");
    auto image=firmware(current.release);tee_identity();TeeTransport backend(image);
    need(sd_notify(0,"READY=1")>=0,"Cannot announce authentication provider readiness");
    std::cout<<"Uke authentication provider ready; waiting for the current kernel challenge\n";
    while(!stopping) {
        const auto request=bridge.current();
        if(request) {
            const bool submitted=process(*request,backend,bridge,[]{return std::chrono::steady_clock::now();});
            std::cout<<(submitted?"Current token submitted; accessory acceptance is a separate result\n":"Expired or changed challenge discarded\n");
            if(mode=="--once")return submitted?0:1;
        } else if(mode=="--once")throw std::runtime_error("No pending kernel authentication challenge");
        bridge.wait();
    }
    return 0;
}
} // namespace uke_pogo_auth
int main(int argc,char **argv) {
    try{return uke_pogo_auth::run(argc,argv);}
    catch(const std::exception &error){if(uke_pogo_auth::stopping)return 0;std::cerr<<"uke-pogo-auth: "<<error.what()<<'\n';return 1;}
}
