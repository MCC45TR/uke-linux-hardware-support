// SPDX-License-Identifier: MIT
// Test-only entry point: disposable QEMU disks, never included in a tablet RPM.
#include "layout-fixture.hpp"
#include <fstream>
#include <iostream>
#include <sys/mount.h>
namespace uke::layout {void vm_power();bool vm_fastboot();}
#define power vm_power
#define fastboot_available vm_fastboot
#include "../src/layout-execute.cpp"
#undef power
#undef fastboot_available
namespace uke::layout {
void vm_power(){need(read("/sys/class/dmi/id/sys_vendor").find("QEMU")!=std::string::npos,"This fixture requires QEMU");}
bool vm_fastboot(){return false;}
}
int main(int argc,char** argv) {
    using namespace fixture;namespace fs=std::filesystem;assert(argc==2);const std::string type=argv[1];
    std::string stage="admission";
    try {
        uke::layout::vm_power();ure::Root system("/");auto target=ure::storage_select(system,"sysfs:"+fs::canonical("/sys/class/block/sda").lexically_relative("/").string());
        stage="private-journal";std::cout<<"VM selected disposable disk\n"<<std::flush;
        const fs::path work="/run/uke-layout-vm";ure::private_directory(work,true);stage="original-gpt-backup";backup(target,work/"original","fixture");
        const std::string dev="/dev/sda30";std::vector<std::string> mkfs;
        if(type=="ext4")mkfs={native_tool("mkfs.ext4"),"-F","-b","4096","-N","2048","-J","size=16","-E","nodiscard,lazy_itable_init=1,lazy_journal_init=1",dev};
        else if(type=="f2fs")mkfs={native_tool("mkfs.f2fs"),"-f","-t","0","-l","UKE_LINUX",dev};
        else if(type=="btrfs")mkfs={native_tool("mkfs.btrfs"),"-f","-K",dev};
        else throw std::runtime_error("Unknown VM filesystem");
        stage="initial-formatter";checked(mkfs);std::cout<<"VM formatted "<<type<<std::endl;
        fs::create_directory("/mnt/userdata");assert(!mount(dev.c_str(),"/mnt/userdata",type.c_str(),MS_NODEV|MS_NOSUID,"nodiscard"));
        {std::ofstream sentinel("/mnt/userdata/keep.txt");sentinel<<"Preserve file through shrink and grow\n";}sync();
        Inspection current;current.target=std::move(target);auto& ev=current.evidence;ev.userdata_index=30;ev.userdata_node=dev;ev.partition_bytes=250*gib-64*mib;ev.filesystem=type;ev.single_device=true;ev.filesystem_size_verified=true;
        auto probe=filesystem_probe(current.target.descriptor.get(),64*mib,ev.partition_bytes);ev.filesystem_uuid=probe["uuid"].asString();ev.filesystem_bytes=probe["filesystem_bytes"].asUInt64();need(probe["size_verified"]==true,"VM filesystem size unavailable");
        auto request=fixture::request();request["userdata_filesystem"]=type;add(request,"esp","512","MiB","fat32");add(request,"linux","40","GiB","ext4");
        ev.userdata_mount="/mnt/userdata";ev.mounted.insert(30);auto proposal=propose(system,current.target,request,work/"original","fixture",{},ev,true,false);
        const bool shutdown=type=="ext4";assert(proposal.document["route"]==(shutdown?"shutdown":"direct"));
        if(shutdown){assert(!umount("/mnt/userdata"));ev.userdata_mount="";ev.mounted.clear();}
        const auto first=work/"split";ure::private_directory(first,true);execute(current,proposal,first,shutdown,[](const auto& phase){std::cout<<"VM split: "<<phase<<std::endl;});
        assert(load_private(first/"state.json")["completed"]==true);
        if(shutdown)assert(!mount(dev.c_str(),"/mnt/userdata",type.c_str(),MS_NODEV|MS_NOSUID,"nodiscard"));
        assert(read("/mnt/userdata/keep.txt")=="Preserve file through shrink and grow\n");std::cout<<"VM preserved sentinel after split\n";
        assert(!umount("/mnt/userdata"));current.target=ure::storage_select(system,"sysfs:"+fs::canonical("/sys/class/block/sda").lexically_relative("/").string());
        ev.partition_bytes=proposal.document["new_rows"][0]["bytes"].asUInt64();probe=filesystem_probe(current.target.descriptor.get(),64*mib,ev.partition_bytes);ev.filesystem_bytes=probe["filesystem_bytes"].asUInt64();
        const bool offlinegrow=type=="f2fs";ev.userdata_mount="";ev.mounted.clear();
        if(!offlinegrow){assert(!mount(dev.c_str(),"/mnt/userdata",type.c_str(),MS_NODEV|MS_NOSUID,"nodiscard"));ev.userdata_mount="/mnt/userdata";ev.mounted.insert(30);}
        auto restore=fixture::request(true);restore["userdata_filesystem"]=type;const auto proof=load_private(first/"ownership.json");auto merged=propose(system,current.target,restore,work/"original","fixture",proof,ev,true,false);assert(merged.document["route"]=="direct");
        const auto second=work/"merge";ure::private_directory(second,true);execute(current,merged,second,offlinegrow,[](const auto& phase){std::cout<<"VM merge: "<<phase<<std::endl;});
        if(offlinegrow)assert(!mount(dev.c_str(),"/mnt/userdata",type.c_str(),MS_NODEV|MS_NOSUID,"nodiscard"));
        assert(read("/mnt/userdata/keep.txt")=="Preserve file through shrink and grow\n");assert(!umount("/mnt/userdata"));
        assert(ure::storage_read(current.target.descriptor.get(),mib,27)=="OEM PAYLOAD MUST NOT CHANGE");
        assert(load_private(second/"state.json")["completed"]==true);std::cout<<"UKE_LAYOUT_VM_PASS "<<type<<" shrink + create + remove + grow + file/OEM preservation\n"<<std::flush;return 0;
    }catch(const std::exception& e){std::cerr<<"UKE_LAYOUT_VM_FAIL at "<<stage<<": "<<e.what()<<std::endl;return 1;}
}
