// SPDX-License-Identifier: MIT
// Disposable guest only. No platform or fault override is installed in an RPM.
#include "layout-vm-common.hpp"
#include <fstream>
#include <iostream>
#include <linux/blkpg.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
static unsigned fail_partition_call=0,partition_calls=0;
static int vm_ioctl(int fd,unsigned long request,void* data=nullptr) {
    if(request==BLKPG&&++partition_calls==fail_partition_call){errno=EBUSY;return -1;}
    return ::ioctl(fd,request,data);
}
namespace uke::layout {
inline bool vm_fastboot(){return false;}
inline Result vm_run(const std::vector<std::string>& args,const std::filesystem::path& cwd={},int inherited=-1,bool terminal=false) {
    auto result=run(args,cwd,inherited,terminal);if(args[0].ends_with("/xfs_growfs"))std::cout<<"VM xfs_growfs: "<<result.output<<std::flush;return result;
}
inline void vm_checked(const std::vector<std::string>& args) {
    auto result=run(args);if(args[0].ends_with("/resize.f2fs"))std::cout<<"VM resize.f2fs: "<<result.output<<std::flush;
    need(result.status==0,"Native VM tool failed: "+result.output);
}
}
#define power vm_power
#define fastboot_available vm_fastboot
#define ioctl vm_ioctl
#define checked vm_checked
#define run vm_run
#include "../src/layout-execute.cpp"
#undef power
#undef fastboot_available
#undef ioctl
#undef checked
#undef run
int main(int argc,char** argv) {
    using namespace fixture;namespace fs=std::filesystem;assert(argc==2);const std::string scenario=argv[1];
    const bool fail_create=scenario=="btrfs-fail-create",fail_merge=scenario=="btrfs-fail-merge";
    const std::string type=(fail_create||fail_merge)?"btrfs":scenario;
    std::string stage="admission";
    try {
        auto current=uke::layout::vm_inspect();ure::Root system("/");const fs::path work="/run/uke-layout-vm";ure::private_directory(work,true);backup(current.target,work/"original","fixture");
        const std::string dev="/dev/sda30";std::vector<std::string> mkfs;
        if(type=="ext4")mkfs={native_tool("mkfs.ext4"),"-F","-b","4096","-N","2048","-J","size=16","-E","nodiscard,lazy_itable_init=1,lazy_journal_init=1",dev};
        else if(type=="f2fs")mkfs={native_tool("mkfs.f2fs"),"-f","-t","0","-l","UKE_LINUX",dev,std::to_string(100*gib/4096)};
        else if(type=="btrfs")mkfs={native_tool("mkfs.btrfs"),"-f","-K",dev};
        else if(type=="xfs")mkfs={native_tool("mkfs.xfs"),"-f","-K","-d","size=100g",dev};
        else throw std::runtime_error("Unknown VM filesystem");
        stage="format";checked(mkfs);fs::create_directory("/mnt/userdata");assert(!mount(dev.c_str(),"/mnt/userdata",type.c_str(),MS_NODEV|MS_NOSUID,"nodiscard"));
        {std::ofstream file("/mnt/userdata/keep.txt");file<<"Preserve file through shrink and grow\n";}sync();
        current=uke::layout::vm_inspect();
        auto r=request();r["userdata_filesystem"]=type;add(r,"esp","512","MiB","fat32");add(r,"linux_boot","512","MiB","ext4");add(r,"linux","40","GiB","ext4");add(r,"linux_swap","1","GiB","linux-swap");add(r,"windows","50","GiB","ntfs");add(r,"shared","1","GiB","exfat");add(r,"linux2","40","GiB","btrfs");
        if(type=="f2fs") {
            assert(current.evidence.filesystem_bytes==100*gib);
            auto full=current.evidence;full.filesystem_bytes=full.partition_bytes;
            assert(propose(system,current.target,r,work/"original","fixture",{},full,true,false).document["route"]=="blocked");
        }
        auto p=propose(system,current.target,r,work/"original","fixture",{},current.evidence,true,false);const bool shutdown=type=="ext4"||type=="f2fs";assert(p.document["route"]==(shutdown?"shutdown":"direct"));
        if(shutdown){assert(!umount("/mnt/userdata"));
        current=uke::layout::vm_inspect();}
        const auto first=work/"split";ure::private_directory(first,true);stage="split";
        if(fail_create){fail_partition_call=3;partition_calls=0;}
        auto check_failed=[&](const fs::path& job,const Proposal& proposal){auto failed=load_private(job/"state.json");assert(failed["completed"]==false&&failed["gpt_state"]=="before"&&failed["kernel_restore_failed"]!=true);for(const auto& row:proposal.document["old_rows"])uke::layout::node(current.target,row);uke::layout::verify(current.target.descriptor.get(),proposal.before);
        assert(read("/mnt/userdata/keep.txt")=="Preserve file through shrink and grow\n");};
        if(fail_create){refuse([&]{execute(current,p,first,false,{});});check_failed(first,p);std::cout<<"UKE_LAYOUT_VM_PASS "<<scenario<<" actual kernel rollback and file/GPT preservation\n";return 0;}
        execute(current,p,first,shutdown,[](const auto& phase){std::cout<<"VM split: "<<phase<<std::endl;});assert(load_private(first/"state.json")["completed"]==true);
        for(const auto* name:{"before-gpt","after-gpt"})assert(ure::gpt_backup_verify(first/name)["verified"]==true);
        if(shutdown)assert(!mount(dev.c_str(),"/mnt/userdata",type.c_str(),MS_NODEV|MS_NOSUID,"nodiscard"));
        assert(read("/mnt/userdata/keep.txt")=="Preserve file through shrink and grow\n");
        current=uke::layout::vm_inspect();const auto matching=propose(system,current.target,r,work/"original","fixture",load_private(first/"ownership.json"),current.evidence,true,false);assert(matching.document["no_changes_required"]==true&&matching.document["confirmations_required"]==0);
        assert(!umount("/mnt/userdata"));const bool offlinegrow=type=="f2fs";
        if(!offlinegrow)assert(!mount(dev.c_str(),"/mnt/userdata",type.c_str(),MS_NODEV|MS_NOSUID,"nodiscard"));
        current=uke::layout::vm_inspect();
        auto restore=request(true);restore["userdata_filesystem"]=type;auto merged=propose(system,current.target,restore,work/"original","fixture",load_private(first/"ownership.json"),current.evidence,true,false);assert(merged.document["route"]=="direct");
        const auto second=work/"merge";ure::private_directory(second,true);stage="merge";
        if(fail_merge){fail_partition_call=3;partition_calls=0;refuse([&]{execute(current,merged,second,false,{});});check_failed(second,merged);std::cout<<"UKE_LAYOUT_VM_PASS "<<scenario<<" actual kernel rollback and file/GPT preservation\n";return 0;}
        execute(current,merged,second,offlinegrow,[](const auto& phase){std::cout<<"VM merge: "<<phase<<std::endl;});
        if(offlinegrow)assert(!mount(dev.c_str(),"/mnt/userdata",type.c_str(),MS_NODEV|MS_NOSUID,"nodiscard"));
        assert(read("/mnt/userdata/keep.txt")=="Preserve file through shrink and grow\n");assert(!umount("/mnt/userdata"));
        assert(ure::storage_read(current.target.descriptor.get(),mib,27)=="OEM PAYLOAD MUST NOT CHANGE");assert(load_private(second/"state.json")["completed"]==true);
        std::cout<<"UKE_LAYOUT_VM_PASS "<<scenario<<" split/merge, all seven child formats, snapshots, file/UUID/OEM preservation\n"<<std::flush;return 0;
    }catch(const std::exception& e){std::cerr<<"UKE_LAYOUT_VM_FAIL at "<<stage<<": "<<e.what()<<std::endl;return 1;}
}
