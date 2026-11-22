// SPDX-License-Identifier: MIT
// The production shutdown hook calls this test-only helper inside disposable QEMU.
#include "layout-vm-common.hpp"
#include <fstream>
#include <iostream>
#include <linux/reboot.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
static long vm_reboot(long,unsigned int,unsigned int,unsigned int,const char*);
namespace uke::layout {inline bool vm_fastboot(){return true;}}
#define power vm_power
#define fastboot_available vm_fastboot
#define inspect vm_inspect
#define syscall vm_reboot
#include "../src/layout-execute.cpp"
#include "../src/layout-shutdown.cpp"
#undef power
#undef fastboot_available
#undef inspect
#undef syscall
namespace {
using namespace uke::layout;
namespace fs=std::filesystem;
bool tamper(){return read("/proc/cmdline").find("uke_test_fs=shutdown-tamper")!=std::string::npos;}
void put(const fs::path& path,const std::string& data){std::ofstream file(path);file<<data;need(bool(file),"Cannot create VM sentinel");}
std::string node_path(const Value& row){return "/dev/sda"+std::to_string(row["index"].asUInt());}
void setup() {
    auto current=vm_inspect();ure::Root system("/");
    checked({native_tool("mkfs.ext4"),"-q","-F","-E","nodiscard","/dev/sdb"});fs::create_directory("/journal");need(!mount("/dev/sdb","/journal","ext4",MS_NODEV|MS_NOSUID,nullptr),"Cannot mount external VM journal");
    const fs::path store="/journal/store";ure::private_directory(store,true);backup(current.target,store/"original-gpt","fixture");
    checked({native_tool("mkfs.ext4"),"-q","-F","-b","4096","-N","2048","-J","size=16","-E","nodiscard,lazy_itable_init=1,lazy_journal_init=1","/dev/sda30"});
    fs::create_directory("/mnt/userdata");need(!mount("/dev/sda30","/mnt/userdata","ext4",MS_NODEV|MS_NOSUID,"nodiscard"),"Cannot mount VM userdata");put("/mnt/userdata/keep.txt","Keep userdata through shutdown\n");need(!umount("/mnt/userdata"),"Cannot release VM userdata");
    current=vm_inspect();auto r=fixture::request();fixture::add(r,"esp","512","MiB","fat32");fixture::add(r,"linux","40","GiB","ext4");
    auto split=propose(system,current.target,r,store/"original-gpt","fixture",{},current.evidence,true,false);ure::private_directory(store/"split",true);execute(current,split,store/"split",true,{});
    fs::create_directory("/oldroot");need(!mount(node_path(split.document["new_rows"][2]).c_str(),"/oldroot","ext4",MS_NODEV|MS_NOSUID,nullptr),"Cannot mount the Linux root that singleboot removes");put("/oldroot/remove.txt","This child is explicitly approved for removal\n");
    fs::create_directory("/oldroot/journal");need(!mount("/journal","/oldroot/journal",nullptr,MS_MOVE,nullptr),"Cannot move the journal below oldroot");
    const fs::path moved="/oldroot/journal/store",job=moved/"job";ure::private_directory(job,true);
    const auto original=ure::private_directory(moved/"original-gpt",false);auto copied=ure::private_directory(job/"original-gpt",true);
    for(const auto& name:original.list(".")) {
        auto input=original.open(name,O_RDONLY),output=copied.open(name,O_WRONLY|O_CREAT|O_EXCL,0600);const auto data=original.read(name,1024*1024);fixture::write(output.get(),data,0);need(!fsync(output.get()),"Cannot persist copied original proof");
    }
    need(!fsync(copied.fd()),"Cannot synchronize copied original proof");auto receipt=load_private(moved/"split/ownership.json");save_private(job/"previous-ownership.json",receipt);
    current=vm_inspect();current.evidence.root_index=split.document["new_rows"][2]["index"].asUInt();auto restore=fixture::request(true);
    auto plan=propose(system,current.target,restore,job/"original-gpt","fixture",receipt,current.evidence,true,true);need(plan.document["route"]=="shutdown"&&plan.document["root_partition_removed"]==true,"VM root removal must require shutdown");save_private(job/"plan.json",plan.document);
    Value consent;consent["schema"]="org.uke.control.layout.consent.v1";consent["plan_sha256"]=plan.document["sha256"];consent["approvals"]=2;consent["boot_id_sha256"]=ure::sha256(read("/proc/sys/kernel/random/boot_id"));consent["expires_ms"]=Json::UInt64(boot_ms()+10*60*1000);consent["journal"]=journal_identity(job,current.target);consent["sha256"]=seal(consent);save_private(job/"consent.json",consent);
    ure::private_directory("/etc/uke-layout",true);save_private("/etc/uke-layout/request.json",uke::layout::shutdown_request(consent,"/run/fixture-stage"));
    if(tamper()){consent["expires_ms"]=Json::UInt64(consent["expires_ms"].asUInt64()+1);consent["sha256"]=seal(consent);save_private(job/"consent.json",consent,true);}
    sync();std::cout<<"UKE_LAYOUT_SHUTDOWN_READY: actual dracut must unmount oldroot and the external journal\n"<<std::flush;
}
}
static long vm_reboot(long number,unsigned int,unsigned int,unsigned int command,const char* argument) {
    using namespace uke::layout;
    need(number==SYS_reboot&&command==LINUX_REBOOT_CMD_RESTART2&&std::string(argument)=="bootloader","Unexpected VM restart request");
    try {
        fs::create_directory("/result-journal");need(!mount("/dev/sdb","/result-journal","ext4",MS_RDONLY|MS_NODEV|MS_NOSUID,nullptr),"Journal was not released before final restart");
        const fs::path job="/result-journal/store/job";auto plan=load_private(job/"plan.json");auto current=vm_inspect();need(current.evidence.mounted.empty(),"Old target root remains mounted at completion");
        if(tamper()) {
            need(!fs::exists(job/"state.json"),"Tampered consent reached storage execution");
            for(const auto& range:plan["before"])need(ure::sha256(ure::storage_read(current.target.descriptor.get(),range["offset"].asUInt64(),range["bytes"].asUInt64()))==range["sha256"].asString(),"Tampered consent changed the GPT");
            std::cout<<"UKE_LAYOUT_VM_PASS shutdown-tamper rejected before target writes\n"<<std::flush;
        } else {
            need(load_private(job/"state.json")["completed"]==true,"Shutdown worker did not complete");
            need(ure::gpt_backup_verify(job/"before-gpt")["verified"]==true&&ure::gpt_backup_verify(job/"after-gpt")["verified"]==true,"Shutdown snapshots are unavailable");
            need(!fs::exists("/sys/class/block/sda34"),"Removed root remains in the kernel partition table");
            need(current.evidence.partition_bytes==fixture::gib*250-fixture::mib*64,"Singleboot did not recover the original userdata capacity");
            need(!mount("/dev/sda30","/mnt/userdata","ext4",MS_RDONLY|MS_NODEV|MS_NOSUID,nullptr),"Cannot read preserved userdata");need(read("/mnt/userdata/keep.txt")=="Keep userdata through shutdown\n","Shutdown lost the userdata sentinel");need(!umount("/mnt/userdata"),"Cannot release preserved userdata");
            need(ure::storage_read(current.target.descriptor.get(),fixture::mib,27)=="OEM PAYLOAD MUST NOT CHANGE","Shutdown modified OEM data");
            std::cout<<"UKE_LAYOUT_VM_PASS shutdown actual oldroot unmount, external journal remount, singleboot and bootloader request\n"<<std::flush;
        }
        need(!umount("/result-journal"),"Cannot release validation journal");sync();::reboot(RB_POWER_OFF);
    }catch(const std::exception& error){std::cerr<<"UKE_LAYOUT_SHUTDOWN_FAIL "<<error.what()<<std::endl;}
    errno=EIO;return -1;
}
int main(int argc,char** argv) {
    try {if(argc==2&&std::string(argv[1])=="--setup"){setup();return 0;}if(argc==2&&std::string(argv[1])=="--offline")return uke::layout::offline();throw std::runtime_error("VM fixture usage");}
    catch(const std::exception& error){std::cerr<<"UKE_LAYOUT_SHUTDOWN_FAIL "<<error.what()<<std::endl;return 1;}
}
