// SPDX-License-Identifier: MIT
#include "layout.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <linux/btrfs.h>
#include <linux/btrfs_tree.h>
#include <linux/fs.h>
#include <signal.h>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/statfs.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <xfs/xfs.h>
namespace uke::layout {
namespace fs=std::filesystem;
void need(bool condition,const std::string& message){if(!condition)throw std::runtime_error(message);}
std::uint64_t boot_ms(){timespec now{};need(!clock_gettime(CLOCK_BOOTTIME,&now),"Cannot read boot elapsed time");return std::uint64_t(now.tv_sec)*1000+now.tv_nsec/1000000;}
std::string read(const fs::path& path,std::size_t limit){
    if(path.string().starts_with("/sys/")||path.string().starts_with("/proc/")) {
        const auto resolved=fs::canonical(path);const auto prefix=path.string().starts_with("/sys/")?"/sys/":"/proc/";
        need(resolved.string().starts_with(prefix),"Kernel observation link escapes its filesystem");return ure::bounded_read(resolved,limit);
    }
    return ure::bounded_read(path,limit);
}
std::string profile(){return "uke-fedora-userdata-v1";}
Value load_private(const fs::path& path) {
    auto root=ure::private_directory(path.parent_path(),false);const auto st=root.stat(path.filename().string());
    need(S_ISREG(st.st_mode)&&st.st_uid==0&&(st.st_mode&0777)==0600&&st.st_nlink==1&&st.st_size>0&&st.st_size<=4*1024*1024,"Untrusted private layout record");
    return ure::parse_json(root.read(path.filename().string(),4*1024*1024));
}
void save_private(const fs::path& path,const Value& data,bool replace){ure::private_directory(path.parent_path(),false).save_record(path.filename().string(),data,replace);}
std::string native_tool(const std::string& name) {
    need(ure::identifier(name),"Invalid native tool name");std::error_code error;
    fs::path alias="/usr/bin/"+name;auto path=fs::canonical(alias,error);if(error){alias="/usr/sbin/"+name;path=fs::canonical(alias,error);}
    need(!error&&(path.parent_path()=="/usr/bin"||path.parent_path()=="/usr/sbin"),"Required native tool is unavailable: "+name);
    ure::Fd fd(open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC));struct stat st{};std::array<char,20> magic{};
    need(fd.get()>=0&&!fstat(fd.get(),&st)&&S_ISREG(st.st_mode)&&st.st_uid==0&&!(st.st_mode&0022)&&(st.st_mode&0111)&&pread(fd.get(),magic.data(),magic.size(),0)==ssize_t(magic.size())&&!memcmp(magic.data(),"\177ELF",4),"Only root-owned native ELF tools are admitted: "+name);
#if defined(__aarch64__)
    need(static_cast<unsigned char>(magic[18])==183&&magic[19]==0,"Foreign native executable");
#elif defined(__x86_64__)
    need(static_cast<unsigned char>(magic[18])==62&&magic[19]==0,"Foreign native executable");
#endif
    // Multicall tools such as resize.f2fs select their operation from argv[0].
    return alias;
}
Result run(const std::vector<std::string>& args,const fs::path& cwd,int inherited,bool terminal) {
    need(!args.empty()&&args[0].starts_with("/usr/"),"Absolute distribution executable required");
    int pipefd[2];need(pipe2(pipefd,O_CLOEXEC)==0,"Cannot open tool output pipe");ure::Fd output(pipefd[0]),writer(pipefd[1]);
    const auto pid=fork();need(pid>=0,"Cannot fork native layout tool");
    if(pid==0) {
        if(!cwd.empty()&&chdir(cwd.c_str()))_exit(126);
        if(!terminal){const int input=open("/dev/null",O_RDONLY);if(input<0||dup2(input,STDIN_FILENO)<0||dup2(writer.get(),STDOUT_FILENO)<0||dup2(writer.get(),STDERR_FILENO)<0)_exit(126);}
        if(inherited>=0&&fcntl(inherited,F_SETFD,0))_exit(126);
        if(prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0))_exit(126);
        clearenv();setenv("PATH","/usr/sbin:/usr/bin",1);setenv("LC_ALL","C.UTF-8",1);setenv("HOME","/",1);setenv("TERM","xterm-256color",1);
        std::vector<char*> argv;for(const auto& arg:args)argv.push_back(const_cast<char*>(arg.c_str()));argv.push_back(nullptr);
        execv(args[0].c_str(),argv.data());_exit(127);
    }
    writer=ure::Fd();std::string log;std::array<char,4096> buffer{};
    while(true){const auto n=::read(output.get(),buffer.data(),buffer.size());if(n<0&&errno==EINTR)continue;need(n>=0,"Cannot read tool output");if(!n)break;if(log.size()<1024*1024)log.append(buffer.data(),std::min(size_t(n),1024*1024-log.size()));}
    int status=0;while(waitpid(pid,&status,0)<0)need(errno==EINTR,"Cannot wait for native tool");
    return {WIFEXITED(status)?WEXITSTATUS(status):128+(WIFSIGNALED(status)?WTERMSIG(status):0),log};
}
void checked(const std::vector<std::string>& args,const fs::path& cwd){const auto r=run(args,cwd);need(r.status==0,"Native tool failed ("+std::to_string(r.status)+"): "+fs::path(args[0]).filename().string()+"\n"+r.output);}
void power() {
    unsigned batteries=0;bool supply=false;
    for(const auto& item:fs::directory_iterator("/sys/class/power_supply")) {
        const auto type=read(item.path()/"type",128);
        if(type=="Battery\n"){++batteries;const auto capacity=std::stoul(read(item.path()/"capacity",32));need(capacity>=50&&capacity<=100,"At least 50% battery is required");const auto health=read(item.path()/"health",128);need(health=="Good\n","Battery health is not good");}
        else if(fs::exists(item.path()/"online")&&read(item.path()/"online",32)=="1\n")supply=true;
    }
    need(batteries==1&&supply,"One healthy battery and external power are required");
}
bool fastboot_available() {
    const fs::path root="/sys/bus/platform/drivers/nvmem-reboot-mode";if(!fs::is_directory(root))return false;
    for(const auto& item:fs::directory_iterator(root)) {
        std::error_code error;auto node=fs::canonical(item.path()/"of_node",error);
        if(error||!node.string().starts_with("/sys/firmware/devicetree/base/"))continue;
        if(fs::is_regular_file(node/"mode-bootloader")&&read(node/"mode-bootloader",16)==std::string("\0\0\0\2",4))return true;
    }return false;
}
void xfs_geometry(Value& probe,const std::string& mount,const std::string& device) {
    ure::Fd fd(open(mount.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC)),disk(open(device.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC));struct stat directory{},block{};xfs_fsop_geom_v1 geometry{};
    need(fd.get()>=0&&disk.get()>=0&&!fstat(fd.get(),&directory)&&!fstat(disk.get(),&block)&&S_ISBLK(block.st_mode)&&directory.st_dev==block.st_rdev,"XFS mount no longer belongs to the admitted partition");
    need(!ioctl(fd.get(),XFS_IOC_FSGEOMETRY_V1,&geometry)&&geometry.blocksize>=512&&geometry.blocksize<=65536&&(geometry.blocksize&(geometry.blocksize-1))==0&&geometry.datablocks>0&&geometry.datablocks<=ure::storage_bytes(disk.get())/geometry.blocksize,"Cannot verify mounted XFS geometry");
    need(geometry.logstart>0&&geometry.rtblocks==0&&probe["ambiguous"]!=true,"External or ambiguous XFS dependencies are outside the userdata boundary");
    std::string uuid;static constexpr char hex[]="0123456789abcdef";
    for(unsigned i=0;i<16;++i){if(i==4||i==6||i==8||i==10)uuid+='-';uuid+=hex[geometry.uuid[i]>>4];uuid+=hex[geometry.uuid[i]&15];}
    need(probe["uuid"].asString().empty()||probe["uuid"]==uuid,"Mounted XFS UUID differs from disk observations");
    // Live XFS can retain the current superblock geometry in its committed log.
    probe["uuid"]=uuid;probe["filesystem_bytes"]=Json::UInt64(geometry.datablocks*geometry.blocksize);probe["size_verified"]=true;
}
static std::string unescape(const std::string& s) {
    std::string out;for(size_t i=0;i<s.size();++i){if(s[i]=='\\'){need(i+3<s.size()&&s[i+1]>='0'&&s[i+1]<='7'&&s[i+2]>='0'&&s[i+2]<='7'&&s[i+3]>='0'&&s[i+3]<='7',"Invalid mountinfo escape");out+=char((s[i+1]-'0')*64+(s[i+2]-'0')*8+s[i+3]-'0');i+=3;}else out+=s[i];}return out;
}
bool btrfs_mount(int fd,const std::string& device,std::uint64_t* devid,std::uint64_t* bytes) {
    btrfs_ioctl_fs_info_args info{};if(fd<0||ioctl(fd,BTRFS_IOC_FS_INFO,&info))return false;
    ure::Fd disk(open(device.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC));need(disk.get()>=0,"Cannot retain Btrfs backing partition");
    const auto raw=ure::storage_read(disk.get(),65536,sizeof(btrfs_super_block));btrfs_super_block super{};memcpy(&super,raw.data(),raw.size());
    if(memcmp(info.fsid,super.fsid,16))return false;
    need(info.num_devices==1&&super.num_devices==1,"Only single-device Btrfs is admitted");
    btrfs_ioctl_dev_info_args dev{};dev.devid=super.dev_item.devid;need(!ioctl(fd,BTRFS_IOC_DEV_INFO,&dev)&&strnlen(reinterpret_cast<const char*>(dev.path),sizeof(dev.path))<sizeof(dev.path),"Cannot inspect Btrfs backing device");
    struct stat actual{},backing{};need(!fstat(disk.get(),&actual)&&!stat(reinterpret_cast<const char*>(dev.path),&backing)&&actual.st_rdev==backing.st_rdev&&S_ISBLK(backing.st_mode),"Btrfs backing device differs from userdata");
    if(devid)*devid=dev.devid;
    if(bytes)*bytes=dev.total_bytes;
    return true;
}
static std::string btrfs_number(int fd) {
    btrfs_ioctl_fs_info_args info{};if(fd<0||ioctl(fd,BTRFS_IOC_FS_INFO,&info)||info.num_devices!=1)return {};
    btrfs_ioctl_dev_info_args dev{};dev.devid=info.max_id;
    need(!ioctl(fd,BTRFS_IOC_DEV_INFO,&dev)&&strnlen(reinterpret_cast<const char*>(dev.path),sizeof(dev.path))<sizeof(dev.path),"Cannot resolve a mounted Btrfs backing device");
    struct stat st{};need(!stat(reinterpret_cast<const char*>(dev.path),&st)&&S_ISBLK(st.st_mode),"Mounted Btrfs backing device is unavailable");return std::to_string(major(st.st_rdev))+":"+std::to_string(minor(st.st_rdev));
}
Inspection inspect() {
    need(geteuid()==0,"Run sudo uke-control singleboot or sudo uke-control multiboot");
    ure::Root system("/");const auto compatible=read("/sys/firmware/devicetree/base/compatible",4096);
    need(compatible.find(std::string("xiaomi,uke\0",11))!=compatible.npos&&compatible.find(std::string("qcom,sm7675\0",12))!=compatible.npos,"Exact Uke / SM7675 identity is required");
    struct stat self{},init{};need(!stat("/proc/self/ns/mnt",&self)&&!stat("/proc/1/ns/mnt",&init)&&self.st_ino==init.st_ino,"System mount namespace required");
    Inspection out;out.graph=ure::storage_graph(system);Value user;
    for(const auto& row:out.graph["objects"])if(row["partition"]==true&&row["label"]=="userdata"){need(user.isNull(),"Ambiguous userdata label");user=row;}
    need(user.isObject(),"Userdata partition is unavailable");
    out.target=ure::storage_select(system,"sysfs:"+user["parent_lun_sysfs"].asString());const auto& id=out.target.identity;
    need(id["kind"]=="live-block"&&id["partition"]==false&&id["unit_identity_available"]==true&&id["read_only_state"]==false&&id["logical_sector_bytes"]==4096&&id["sysfs_path"].asString().find("/1d84000.ufshc/")!=std::string::npos,"A writable identified Uke UFS LUN with 4 KiB sectors is required");
    return inspect_storage(std::move(out.target),out.graph,user);
}
Inspection inspect_storage(StorageTarget target,const Value& graph,const Value& user) {
    Inspection out;out.target=std::move(target);out.graph=graph;const auto& id=out.target.identity;
    auto& ev=out.evidence;ev.userdata_index=user["partition_index"].asUInt();ev.userdata_node="/dev/"+user["kernel_name"].asString();ev.partition_bytes=user["bytes"].asUInt64();
    const auto at=user["start_512_sectors"].asUInt64()*512;auto probe=filesystem_probe(out.target.descriptor.get(),at,ev.partition_bytes);
    ev.filesystem=probe["type"].asString();ev.filesystem_uuid=probe["uuid"].asString();ev.filesystem_bytes=probe["filesystem_bytes"].asUInt64();ev.filesystem_size_verified=probe["size_verified"].asBool();ev.external_journal=probe["external_journal"].asBool();ev.single_device=probe["single_device"]!=false;
    std::map<std::string,unsigned> numbers;
    for(const auto& row:out.graph["objects"])if(row["parent_lun_sysfs"]==id["sysfs_path"]&&row["partition"]==true) {
        const auto index=row["partition_index"].asUInt();numbers[row["device_number"].asString()]=index;
        if(!row["holders"].empty())ev.holders.insert(index);
        need(row["dependencies_available"]==true,"Incomplete block dependency inventory");
    }
    std::istringstream mounts(read("/proc/self/mountinfo",4*1024*1024));std::string line;
    while(std::getline(mounts,line)) {
        std::istringstream fields(line);std::string a,b,number,root,mount,options;need(bool(fields>>a>>b>>number>>root>>mount>>options),"Malformed mount inventory");
        mount=unescape(mount);unsigned index=0;
        if(numbers.contains(number))index=numbers.at(number);
        else if(line.find(" - btrfs ")!=line.npos){ure::Fd fd(open(mount.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));const auto backing=btrfs_number(fd.get());if(numbers.contains(backing))index=numbers.at(backing);}
        if(!index)continue;
        ev.mounted.insert(index);
        if(mount=="/")ev.root_index=index;
        if(index==ev.userdata_index){need(options.find("rw")!=options.npos,"Read-only userdata cannot be resized live");if(ev.userdata_mount.empty()||mount=="/")ev.userdata_mount=mount;}
    }
    std::istringstream swaps(read("/proc/swaps"));std::getline(swaps,line);
    while(std::getline(swaps,line)) {
        std::istringstream fields(line);std::string path;fields>>path;path=unescape(path);struct stat st{};need(!stat(path.c_str(),&st),"Cannot resolve an active swap");auto number=std::to_string(major(S_ISBLK(st.st_mode)?st.st_rdev:st.st_dev))+":"+std::to_string(minor(S_ISBLK(st.st_mode)?st.st_rdev:st.st_dev));
        if(numbers.contains(number))ev.swaps.insert(numbers.at(number));
        else if(S_ISREG(st.st_mode)){ure::Fd fd(open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW));const auto backing=btrfs_number(fd.get());if(numbers.contains(backing))ev.swaps.insert(numbers.at(backing));}
    }
    auto diskread=[&](std::uint64_t offset,std::size_t n){return ure::storage_read(out.target.descriptor.get(),at+offset,n);};
    if(ev.filesystem=="btrfs") {
        const auto raw=diskread(65536,sizeof(btrfs_super_block));btrfs_super_block super{};memcpy(&super,raw.data(),raw.size());
        ev.single_device=std::endian::native==std::endian::little&&super.num_devices==1;
        ev.filesystem_bytes=super.dev_item.total_bytes;ev.filesystem_size_verified=ev.single_device&&ev.filesystem_bytes>0&&ev.filesystem_bytes<=ev.partition_bytes;
        if(!ev.userdata_mount.empty()) {
            ure::Fd fd(open(ev.userdata_mount.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));btrfs_ioctl_fs_info_args info{};btrfs_ioctl_dev_info_args dev{};dev.devid=super.dev_item.devid;
            need(fd.get()>=0&&!ioctl(fd.get(),BTRFS_IOC_FS_INFO,&info)&&info.num_devices==1&&!ioctl(fd.get(),BTRFS_IOC_DEV_INFO,&dev),"Cannot prove single-device Btrfs ownership");
            struct stat backing{},actual{};need(!stat(reinterpret_cast<const char*>(dev.path),&backing)&&!stat(ev.userdata_node.c_str(),&actual)&&backing.st_rdev==actual.st_rdev&&!memcmp(info.fsid,super.fsid,16),"Btrfs backing device differs from userdata");
            ev.filesystem_bytes=dev.total_bytes;
        }
    } else if(ev.filesystem=="xfs"&&!ev.userdata_mount.empty()) {
        xfs_geometry(probe,ev.userdata_mount,ev.userdata_node);ev.filesystem_uuid=probe["uuid"].asString();ev.filesystem_bytes=probe["filesystem_bytes"].asUInt64();ev.filesystem_size_verified=true;
    } else if(ev.filesystem=="f2fs") {
        // Linux 7.3 f2fs_super_block::devs: eight 68-byte descriptors after encryption fields.
        for(auto start:{1024U,5120U}){const auto devs=diskread(start+2201,8*68);ev.single_device=ev.single_device&&std::all_of(devs.begin(),devs.end(),[](char c){return c==0;});}
    }
    ev.require_shutdown=fs::exists("/run/uke-control-layout-require-shutdown");
    out.summary["device"]="Uke";out.summary["userdata_filesystem"]=ev.filesystem;out.summary["userdata_bytes"]=Json::UInt64(ev.partition_bytes);out.summary["filesystem_bytes"]=Json::UInt64(ev.filesystem_bytes);out.summary["filesystem_size_verified"]=ev.filesystem_size_verified;out.summary["single_device"]=ev.single_device;out.summary["fastboot_available"]=fastboot_available();out.summary["filesystems"]=native_capabilities();out.summary["physical_test_record"]=false;
    return out;
}
Value journal_identity(const fs::path& path,const StorageTarget& target) {
    ure::private_directory(path,false);struct stat st{};need(!stat(path.c_str(),&st),"Cannot inspect journal volume");const auto number=std::to_string(major(st.st_dev))+":"+std::to_string(minor(st.st_dev));
    auto sys=fs::canonical("/sys/dev/block/"+number);const auto target_sys=fs::path("/"+target.identity["sysfs_path"].asString());
    need(sys!=target_sys&&sys.parent_path()!=target_sys&&sys.string().find("/virtual/")==std::string::npos&&fs::is_empty(sys/"holders")&&(!fs::exists(sys/"slaves")||fs::is_empty(sys/"slaves")),"The durable journal must be on a separate direct block device");
    const auto name=sys.filename().string();ure::Fd fd(open(("/dev/"+name).c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC));need(fd.get()>=0,"Cannot inspect external journal filesystem");auto probe=filesystem_probe(fd.get(),0,ure::storage_bytes(fd.get()));
    const auto type=probe["type"].asString();need((type=="ext4"||type=="xfs")&&!probe["uuid"].asString().empty()&&probe["external_journal"]!=true,"Journal requires an external native ext4 or XFS filesystem with a unique UUID and internal journal");
    std::istringstream mounts(read("/proc/self/mountinfo",4*1024*1024));std::string line,mountpoint;
    while(std::getline(mounts,line)){std::istringstream fields(line);std::string a,b,dev,root,mount;fields>>a>>b>>dev>>root>>mount;if(dev==number){need(root=="/","Journal must use the filesystem root, without bind mounts");mountpoint=unescape(mount);break;}}
    need(!mountpoint.empty(),"External journal mount is unavailable");auto rel=fs::canonical(path).lexically_relative(mountpoint);need(!rel.empty()&&!rel.string().starts_with(".."),"Journal escapes its external mount");
    Value out;out["uuid"]=probe["uuid"];out["filesystem"]=type;out["relative"]=rel.string();out["device_sysfs"]=sys.string();return out;
}
}
