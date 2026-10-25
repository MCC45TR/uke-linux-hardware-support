// SPDX-License-Identifier: MIT
#include "layout.hpp"
#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <linux/magic.h>
#include <linux/reboot.h>
#include <sstream>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <unistd.h>
namespace uke::layout {
namespace fs=std::filesystem;
namespace {
void text(const fs::path& path,const std::string& value,mode_t mode=0600) {
    ure::Fd fd(open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,mode));need(fd.get()>=0,"Cannot create shutdown staging file");size_t done=0;
    while(done<value.size()){const auto n=write(fd.get(),value.data()+done,value.size()-done);if(n<0&&errno==EINTR)continue;need(n>0,"Cannot write shutdown staging file");done+=size_t(n);}need(!fsync(fd.get()),"Cannot synchronize shutdown staging file");
}
void audit(const fs::path& root) {
    std::uintmax_t total=0;
    for(const auto& entry:fs::recursive_directory_iterator(root)) {
        const auto path=entry.path().lexically_relative(root).string();const auto status=entry.symlink_status();
        need(path.find("python")==path.npos&&path.find("pypy")==path.npos&&!path.ends_with(".py")&&!path.ends_with(".pyc"),"Python is forbidden in layout maintenance RAM");
        if(fs::is_regular_file(status)){total+=entry.file_size();need(total<=128*1024*1024,"Layout maintenance RAM exceeds 128 MiB");}
        else if(!fs::is_symlink(status)&&!fs::is_directory(status)) {
            struct stat st{};need(!lstat(entry.path().c_str(),&st)&&S_ISCHR(st.st_mode)&&st.st_uid==0&&st.st_nlink==1,"Unexpected special file in layout maintenance RAM");
            bool admitted=false;for(const auto& item:std::vector<std::pair<const char*,dev_t>>{{"dev/null",makedev(1,3)},{"dev/random",makedev(1,8)},{"dev/urandom",makedev(1,9)},{"dev/kmsg",makedev(1,11)},{"dev/console",makedev(5,1)}})admitted=admitted||(path==item.first&&st.st_rdev==item.second);
            need(admitted,"Unexpected device in layout maintenance RAM");
        }
    }
    need(!fs::exists(root/"etc/shadow")&&fs::exists(root/"usr/lib/initrd-release")&&access((root/"shutdown").c_str(),X_OK)==0&&access((root/"usr/libexec/uke-control/layout-manager").c_str(),X_OK)==0,"Incomplete or private-account-bearing RAM environment");
}
void watchdogs() {
    if(!fs::exists("/sys/class/watchdog"))return;
    for(const auto& item:fs::directory_iterator("/sys/class/watchdog"))need(read(item.path()/"state",64)!="active\n","An active hardware watchdog has no admitted RAM keeper; offline maintenance is blocked");
}
void label(const fs::path& root) {
    if(!fs::exists("/sys/fs/selinux/enforce"))return;
    std::istringstream config(read("/etc/selinux/config"));std::string line,type;
    while(std::getline(config,line))if(line.starts_with("SELINUXTYPE=")){need(type.empty(),"Duplicate SELinux policy type");type=line.substr(12);if(type.size()>1&&type.front()=='\"'&&type.back()=='\"')type=type.substr(1,type.size()-2);}
    need(ure::identifier(type),"Unknown SELinux policy type");checked({"/usr/sbin/setfiles","-F","-r",root,"/etc/selinux/"+type+"/contexts/files/file_contexts",root});
}
}
void build_shutdown_payload(const fs::path& stage,const Value& request) {
    ure::private_directory(stage,false);fs::create_directory(stage/"empty");fs::create_directory(stage/"tree");
    save_private(stage/"request.json",request);
    text(stage/"hook","#!/bin/sh\n# One attempt, after dracut has unmounted oldroot. The native worker performs the final restart.\n/usr/libexec/uke-control/layout-manager --offline\nwarn 'Uke layout maintenance stopped; no automatic retry'\npoweroff -f\nwhile :; do read -r _ </dev/console; done\n",0755);
    std::string tools="/usr/libexec/uke-control/layout-manager "+native_tool("dmsetup")+" "+native_tool("wipefs");
    for(const auto& cap:native_capabilities())if(cap["available"]==true){tools+=" "+cap["formatter"].asString();tools+=" "+cap["checker"].asString();}
    for(const auto* name:{"resize2fs","resize.f2fs","xfs_growfs","blkid"}){try{tools+=" "+native_tool(name);}catch(const std::exception&){}}
    checked({"/usr/bin/dracut","--conf","/dev/null","--confdir",stage/"empty","--tmpdir",stage,"--no-kernel","--no-hostonly","--no-hostonly-cmdline","--no-hostonly-default-device","--no-early-microcode","--no-compress","--modules","base shutdown","--install",tools,"--include",stage/"request.json","/etc/uke-layout/request.json","--include",stage/"hook","/var/lib/dracut/hooks/shutdown/20-uke-layout.sh",stage/"layout.cpio"});
    checked({"/usr/bin/lsinitrd","--unpack",stage/"layout.cpio"},stage/"tree");
    need(!chmod((stage/"tree/etc/uke-layout").c_str(),0700),"Cannot protect RAM request directory");audit(stage/"tree");
}
void prepare_shutdown(const fs::path& job) {
    auto plan=load_private(job/"plan.json");auto consent=load_private(job/"consent.json");need(plan["route"]=="shutdown"&&plan["sha256"]==seal(plan)&&consent["plan_sha256"]==plan["sha256"]&&consent["sha256"]==seal(consent),"Shutdown preparation requires a confirmed plan");
    need(plan["engine_version"]==engine_version&&plan["recovery_source"]==recovery_source&&consent["approvals"]==plan["confirmations_required"]&&consent["boot_id_sha256"]==ure::sha256(read("/proc/sys/kernel/random/boot_id"))&&consent["expires_ms"].asUInt64()>=ure::monotonic_ms()&&!fs::exists(job/"state.json"),"Shutdown approval is stale, incomplete or already used");
    auto current=inspect();power();watchdogs();need(fastboot_available(),"Bound bootloader restart support is required");
    auto actual=journal_identity(job,current.target);need(actual["uuid"]==consent["journal"]["uuid"]&&actual["relative"]==consent["journal"]["relative"],"External journal identity changed");
    struct statfs ram{};need(!statfs("/run",&ram)&&ram.f_type==TMPFS_MAGIC,"Native RAM at /run is required");
    ure::Fd lock(open("/run/uke-control-layout.lock",O_RDWR|O_CREAT|O_CLOEXEC|O_NOFOLLOW,0600));need(lock.get()>=0&&!flock(lock.get(),LOCK_EX|LOCK_NB),"Another layout task is active");
    const fs::path destination="/run/initramfs";if(!fs::exists(destination))need(!mkdir(destination.c_str(),0700),"Cannot create shutdown directory");
    struct stat directory{};need(!lstat(destination.c_str(),&directory)&&S_ISDIR(directory.st_mode)&&directory.st_uid==0&&!(directory.st_mode&0022),"Untrusted existing shutdown directory");
    for(const auto* existing:{"shutdown","bin/sh",".need_shutdown","etc/uke-layout/request.json"})need(!fs::exists(destination/existing),"An existing shutdown environment is preserved; resolve it before layout maintenance");
    checked({native_tool("modprobe"),"dm_mod"});
    if(plan["preserve_userdata"]==true)checked({native_tool("modprobe"),plan["userdata_filesystem"].asString()});
    char pattern[]="/run/uke-layout-stage-XXXXXX";const auto created=mkdtemp(pattern);need(created,"Cannot allocate private RAM staging");const fs::path stage=created;
    Value ram_request=consent;ram_request["plan_sha256"]=plan["sha256"];ram_request["stage"]=stage.string();ram_request["sha256"]=seal(ram_request);
    build_shutdown_payload(stage,ram_request);label(stage/"tree");power();
    // Renew the one-shot lease only after the RAM payload has passed audit.
    consent["expires_ms"]=Json::UInt64(ure::monotonic_ms()+10*60*1000);consent["sha256"]=seal(consent);save_private(job/"consent.json",consent,true);
    ram_request=consent;ram_request["stage"]=stage.string();ram_request["sha256"]=seal(ram_request);save_private(stage/"tree/etc/uke-layout/request.json",ram_request,true);
    fs::rename(destination,stage/"previous");try{fs::rename(stage/"tree",destination);}catch(...){fs::rename(stage/"previous",destination);throw;}
    std::cout<<"One shutdown task is ready in RAM. No disk data has been changed.\n";
}
void withdraw_shutdown(const fs::path& job) {
    const auto request=load_private("/run/initramfs/etc/uke-layout/request.json"),plan=load_private(job/"plan.json");need(request["plan_sha256"]==plan["sha256"]&&request["sha256"]==seal(request),"A different shutdown task is preserved");
    const fs::path stage=request["stage"].asString();need(stage.parent_path()=="/run"&&stage.filename().string().starts_with("uke-layout-stage-"),"Invalid RAM staging locator");ure::private_directory(stage,false);
    fs::rename("/run/initramfs",stage/"withdrawn");fs::rename(stage/"previous","/run/initramfs");
    std::cerr<<"The reboot request was refused. The layout task was withdrawn; no storage change ran.\n";
}
int offline() {
    bool mounted=false;
    try {
        struct statfs ram{};need(!statfs("/",&ram)&&ram.f_type==TMPFS_MAGIC&&fs::exists("/usr/lib/initrd-release"),"Layout work requires the shutdown RAM root");
        const auto request=load_private("/etc/uke-layout/request.json");need(request["sha256"]==seal(request)&&request["boot_id_sha256"]==ure::sha256(read("/proc/sys/kernel/random/boot_id"))&&request["expires_ms"].asUInt64()>=ure::monotonic_ms(),"Shutdown request is stale or belongs to another boot");
        watchdogs();power();auto current=inspect();const auto& locator=request["journal"];
        std::string external;unsigned found=0;
        for(const auto& item:fs::directory_iterator("/sys/class/block")) {
            const auto sys=fs::canonical(item.path());if(sys.string()!=locator["device_sysfs"].asString())continue;
            const auto path="/dev/"+item.path().filename().string();ure::Fd fd(open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC));need(fd.get()>=0,"External journal device disappeared");const auto probe=filesystem_probe(fd.get(),0,ure::storage_bytes(fd.get()));
            if(probe["uuid"]==locator["uuid"]&&probe["type"]==locator["filesystem"]){external=path;++found;}
        }
        need(found==1,"Exactly one unchanged external journal device is required");
        const fs::path mountpoint="/run/uke-layout-journal";ure::private_directory(mountpoint,!fs::exists(mountpoint));
        need(!mount(external.c_str(),mountpoint.c_str(),locator["filesystem"].asCString(),MS_NODEV|MS_NOSUID|MS_NOEXEC,nullptr),"Cannot mount durable journal after oldroot unmount");mounted=true;
        const auto relative=locator["relative"].asString();ure::components(relative);const auto job=mountpoint/relative;auto consent=load_private(job/"consent.json");
        need(consent["plan_sha256"]==request["plan_sha256"]&&consent["sha256"]==seal(consent),"Durable and RAM confirmations differ");
        run_worker(job/"consent.json",true,[](const auto& phase){std::cout<<"Uke layout: "<<phase<<std::endl;});
        sync();need(!umount(mountpoint.c_str()),"Cannot release durable journal");mounted=false;
        need(fastboot_available(),"Bootloader restart support disappeared");syscall(SYS_reboot,LINUX_REBOOT_MAGIC1,LINUX_REBOOT_MAGIC2,LINUX_REBOOT_CMD_RESTART2,"bootloader");throw std::runtime_error("Bootloader restart returned unexpectedly");
    } catch(const std::exception& error) {
        std::cerr<<"Uke layout stopped: "<<error.what()<<"\nRetain the external journal. No automatic retry.\n"<<std::flush;sync();if(mounted)umount("/run/uke-layout-journal");
        if(fastboot_available())syscall(SYS_reboot,LINUX_REBOOT_MAGIC1,LINUX_REBOOT_MAGIC2,LINUX_REBOOT_CMD_RESTART2,"bootloader");
        reboot(RB_POWER_OFF);return 1;
    }
}
}
