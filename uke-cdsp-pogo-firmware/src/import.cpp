// SPDX-License-Identifier: MIT
#include "fat.hpp"
namespace uke_local_fw {
using Family=std::map<std::string,Bytes>;
static constexpr const char *mcu_sha="b17fa938cf5613dfa550179d6fc40cfbaa831d534a16a22282e2904e845fa7c4";
static constexpr const char *main_sha="cdd2ae8de5b47b534439ccdf2ebc696123f42f166990dd704601ca5a3cc03e39";
static constexpr const char *dtb_sha="cf0a4f5e994bb766fb2edbc2c1b0ce39ca9aff0ff127de0fe01e587d7a919dbb";
static uint16_t le16(const Bytes &b,size_t p) {
    need(p<=b.size()&&b.size()-p>=2,"Truncated firmware field");return b[p]|uint16_t(b[p+1])<<8;
}
static uint32_t le32(const Bytes &b,size_t p) {return le16(b,p)|uint32_t(le16(b,p+2))<<16;}
static void admit_mdt(const Bytes &b,const char *hash) {
    need(b.size()<=1024*1024&&b.size()>=52&&digest(b)==hash,"Stock CDSP MDT identity is not admitted");
    need(!memcmp(b.data(),"\177ELF",4)&&b[4]==1&&b[5]==1&&b[6]==1&&le16(b,40)==52&&
         le16(b,42)==32&&le16(b,44)>0&&le16(b,44)<=64,"Stock CDSP ELF32 layout differs");
    const uint64_t ph=le32(b,28),count=le16(b,44);
    need(ph>=52&&ph<=b.size()&&count*32<=b.size()-ph,"CDSP program table exceeds MDT bounds");
}
static bool admit_mcu(const Bytes &b) {return b.size()==82740&&digest(b)==mcu_sha;}
static Family cdsp_family(const Fat &fat) {
    Family out;
    for(const auto &item:std::array<std::pair<std::string,const char *>,2>{{{"cdsp",main_sha},{"cdsp_dtb",dtb_sha}}}) {
        auto upper=item.first;std::transform(upper.begin(),upper.end(),upper.begin(),[](unsigned char c){return c>='a'&&c<='z'?c-32:c;});
        auto mdt=fat.image_file(upper+".MDT",1024*1024);admit_mdt(mdt,item.second);
        const size_t ph=le32(mdt,28),count=le16(mdt,44);
        uint64_t total=0;const uint64_t maximum=item.first=="cdsp"?0x1400000:0x80000;
        for(size_t i=0;i<count;i++) {
            const auto p=ph+i*32;
            const auto offset=le32(mdt,p+4),bytes=le32(mdt,p+16),flags=le32(mdt,p+24);
            if(!bytes||(le32(mdt,p)!=1&&(flags&0x07000000)!=0x02000000))continue;
            need(bytes<=maximum,"CDSP split image exceeds its reserved region");
            if(offset<=mdt.size()&&bytes<=mdt.size()-offset)continue;
            total+=bytes;need(total<=maximum+1024*1024,"CDSP split family is oversized");
            char suffix[8];snprintf(suffix,sizeof(suffix),".b%02u",unsigned(i));
            std::string name=item.first+suffix,source=upper+suffix;
            std::transform(source.begin(),source.end(),source.begin(),[](unsigned char c){return c>='a'&&c<='z'?c-32:c;});
            auto payload=fat.image_file(source,maximum);
            need(payload.size()==bytes,"CDSP split image size differs from its authenticated metadata");
            out.emplace(name,std::move(payload));
        }
        out.emplace(item.first+".mdt",std::move(mdt));
    }
    return out;
}
static void verify_family(const fs::path &path,const Family &family) {
    auto dir=directory(path);
    for(const auto &[name,bytes]:family) {
        need(token(name)&&fs::path(name).filename()==name,"Invalid fixed firmware filename");
        need(file(path/name)==bytes,"Existing versioned firmware differs; it was not replaced");
    }
}
static void publish_family(const fs::path &parent,const std::string &kind,const Family &family) {
    need((kind=="cdsp"||kind=="nanosic")&&!family.empty(),"Invalid firmware family");
    auto dir=directory(parent,true);const auto final=parent/kind;
    if(fs::exists(final)||fs::is_symlink(final)){verify_family(final,family);return;}
    uint64_t random=0;need(getrandom(&random,sizeof(random),0)==ssize_t(sizeof(random)),"Cannot create firmware stage name");
    const auto name=".uke-local-"+std::to_string(random);
    need(!mkdirat(dir.value,name.c_str(),0700),"Cannot create protected firmware stage");
    Fd stage(openat(dir.value,name.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));
    struct Cleanup {
        int parent,stage;std::string name;std::vector<std::string> files;bool active=true;
        ~Cleanup(){if(active){for(const auto &p:files)unlinkat(stage,p.c_str(),0);unlinkat(parent,name.c_str(),AT_REMOVEDIR);}}
    } cleanup{dir.value,stage.value,name,{}};
    for(const auto &[file_name,bytes]:family) {
        need(token(file_name)&&fs::path(file_name).filename()==file_name&&!bytes.empty()&&bytes.size()<=24*1024*1024,
             "Invalid staged firmware file");
        Fd out(openat(stage.value,file_name.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600));
        cleanup.files.push_back(file_name);size_t done=0;
        while(done<bytes.size()) {
            const auto n=write(out.value,bytes.data()+done,bytes.size()-done);
            if(n<0&&errno==EINTR)continue;need(n>0,"Cannot stage local firmware");done+=size_t(n);
        }
        need(!fchmod(out.value,0644)&&!fsync(out.value),"Cannot sync local firmware");
        Fd readback(openat(stage.value,file_name.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW));
        need(read_fd(readback.value,bytes.size())==bytes,"Firmware stage readback differs");
    }
    need(!fsync(stage.value),"Cannot sync firmware stage directory");
    if(syscall(SYS_renameat2,dir.value,name.c_str(),dir.value,kind.c_str(),RENAME_NOREPLACE)<0) {
        need(errno==EEXIST,"Cannot publish versioned firmware");verify_family(final,family);
    } else cleanup.active=false;
    need(!fsync(dir.value),"Cannot sync firmware parent");verify_family(final,family);
}
#ifndef UKE_LOCAL_FW_TEST
static Bytes sysfile(const fs::path &path,size_t maximum) {
    Fd fd(open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW));Bytes out(maximum);
    const auto n=read(fd.value,out.data(),out.size());need(n>0&&size_t(n)<maximum,"Unexpected sysfs identity length");
    out.resize(n);return out;
}
static bool compatible(const Bytes &b,const std::string &expected) {
    size_t p=0;
    while(p<b.size()) {
        const auto e=std::find(b.begin()+p,b.end(),0);need(e!=b.end(),"Unterminated board identity");
        const std::string value(b.begin()+p,e);if(value==expected)return true;p=size_t(e-b.begin())+1;
    }
    return false;
}
struct Source {Fd fd;uint64_t size;dev_t device;fs::path parent;};
static Source source(const char *label) {
    const auto path=fs::canonical(fs::path("/dev/disk/by-partlabel")/label);
    need(path.parent_path()=="/dev","Unexpected stock partition symlink target");
    Fd fd(open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK));struct stat st{};uint64_t size=0;
    need(!fstat(fd.value,&st)&&S_ISBLK(st.st_mode)&&!ioctl(fd.value,BLKGETSIZE64,&size)&&size,
         "Stock source is not a readable block partition");
    const auto sys=fs::canonical("/sys/dev/block/"+std::to_string(major(st.st_rdev))+":"+std::to_string(minor(st.st_rdev)));
    need(fs::exists(sys/"partition"),"Stock source is not a partition");
    return {std::move(fd),size,st.st_rdev,sys.parent_path()};
}
static Bytes mcu_image(Source &super) {
    trusted("/usr/bin/debugfs");const auto parts=uke_video::partitions(super.fd.value,super.size);
    for(const auto &label:{"odm_a","vendor_a"}) {
        const auto found=parts.find(label);if(found==parts.end())continue;
        auto [loop,path]=uke_video::readonly_loop(super.fd.value,found->second);
        const auto bytes=uke_video::capture({"/usr/bin/debugfs","-R","cat /firmware/MCU_Upgrade.bin",path},1024*1024);
        if(admit_mcu(bytes))return bytes;
    }
    throw std::runtime_error("No hash-admitted Uke MCU image found in stock ODM/vendor");
}
static std::pair<Family,Bytes> acquire() {
    need(geteuid()==0,"Administrator authentication is required for read-only stock partition access");
    const auto board=sysfile("/sys/firmware/devicetree/base/compatible",1024);
    need(compatible(board,"xiaomi,uke")&&compatible(board,"qcom,sm7675"),"Exact Uke/SM7675 identity required");
    auto modem=source("modem_a"),super=source("super");
    need(modem.parent==super.parent,"Stock sources belong to different physical disks");
    struct stat root_stat{};need(!stat("/",&root_stat),"Cannot identify Fedora root filesystem");
    const auto root_sys=fs::canonical("/sys/dev/block/"+std::to_string(major(root_stat.st_dev))+":"+std::to_string(minor(root_stat.st_dev)));
    need(root_sys.parent_path()==super.parent,"Stock sources are outside the Fedora root disk");
    Fat fat(modem.fd.value,modem.size);auto cdsp=cdsp_family(fat);auto mcu=mcu_image(super);
    need(admit_mcu(mcu),"MCU identity differs");
    return {std::move(cdsp),std::move(mcu)};
}
static int preflight() {
    const auto files=acquire();
    std::cout<<"Verified device-local CDSP metadata/split files and exact MCU image; no files were published or devices started\n";
    return 0;
}
static int run(const std::string &release) {
    need(token(release)&&release.starts_with("7.3.1-"),"Invalid target kernel release");
    utsname current{};need(!uname(&current)&&release!=current.release,"Only a non-running kernel may receive firmware");
    need(fs::is_directory(fs::path("/usr/lib/modules")/release),"Target kernel modules are not installed");
    const auto [cdsp,mcu]=acquire();
    const auto parent=fs::path("/usr/lib/firmware")/release/"qcom/uke";
    publish_family(parent,"cdsp",cdsp);publish_family(parent,"nanosic",Family{{"MCU_Upgrade.bin",mcu}});
    std::cout<<"Verified and published device-local CDSP and MCU firmware for "<<release
             <<"; source partitions stayed read-only; no DSP or MCU was started\n";
    return 0;
}
#endif
}
#ifndef UKE_LOCAL_FW_TEST
int main(int argc,char **argv) {
    try {
        if(argc==2&&std::string(argv[1])=="--preflight")return uke_local_fw::preflight();
        need(argc==3&&std::string(argv[1])=="--import","Usage: uke-cdsp-pogo-firmware --preflight | --import KERNEL_RELEASE");
        return uke_local_fw::run(argv[2]);}
    catch(const std::exception &e){std::cerr<<"uke-cdsp-pogo-firmware: "<<e.what()<<'\n';return 1;}
}
#endif
