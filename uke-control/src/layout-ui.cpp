// SPDX-License-Identifier: MIT
#include "layout.hpp"
#include "cli.hpp"
#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
namespace uke::layout {
namespace fs=std::filesystem;
namespace {
struct Cancel {};
std::string dialog(const std::vector<std::string>& options) {
    char temporary[]="/run/uke-layout-ui-XXXXXX";ure::Fd fd(mkstemp(temporary));need(fd.get()>=0,"Cannot allocate private terminal UI output");unlink(temporary);
    std::vector<std::string> args{native_tool("dialog"),"--mouse","--backtitle","Uke storage manager","--no-collapse","--output-fd",std::to_string(fd.get())};args.insert(args.end(),options.begin(),options.end());
    const auto result=run(args,{},fd.get(),true);if(result.status==1||result.status==255)throw Cancel{};need(result.status==0,"Terminal dialog failed");
    need(lseek(fd.get(),0,SEEK_SET)==0,"Cannot read terminal selection");std::string out;char buffer[1024];ssize_t n;while((n=::read(fd.get(),buffer,sizeof(buffer)))>0){need(out.size()+size_t(n)<=16384,"Oversized terminal selection");out.append(buffer,size_t(n));}need(n==0,"Cannot read terminal selection");return out;
}
std::string input(const std::string& prompt,const std::string& initial={}){return dialog({"--inputbox",prompt,"12","78",initial});}
std::string menu(const std::string& prompt,const std::vector<std::string>& pairs) {
    std::vector<std::string> args{"--menu",prompt,"22","90","13"};args.insert(args.end(),pairs.begin(),pairs.end());return dialog(args);
}
std::string menu(const std::string& prompt,std::initializer_list<const char*> pairs) {
    std::vector<std::string> options;for(const auto* pair:pairs)options.emplace_back(pair);return menu(prompt,options);
}
void notice(const std::string& message){dialog({"--msgbox",message,"24","95"});}
bool yes(const std::string& message){try{dialog({"--defaultno","--yesno",message,"20","90"});return true;}catch(const Cancel&){return false;}}
void trusted(const fs::path& directory) {
    need(directory.is_absolute()&&directory==directory.lexically_normal(),"Use an absolute normalized journal path");fs::path current="/";
    for(const auto& part:directory.relative_path()){current/=part;struct stat st{};need(!lstat(current.c_str(),&st)&&S_ISDIR(st.st_mode)&&st.st_uid==0&&!(st.st_mode&0022),"Journal ancestors must be real root-owned directories without group/other write permission");}
}
void copy_original(const fs::path& source,const fs::path& destination) {
    ure::gpt_backup_verify(source);auto out=ure::private_directory(destination,true);auto in=ure::private_directory(source,false);
    for(const auto* name:{"manifest.json","partition-table.json","SHA256SUMS","protective_mbr.bin","primary_header.bin","primary_table.bin","backup_header.bin","backup_table.bin"}) {
        const auto data=in.read(name,4*1024*1024);auto file=out.open(name,O_WRONLY|O_CREAT|O_EXCL,0600);size_t done=0;
        while(done<data.size()){const auto n=write(file.get(),data.data()+done,data.size()-done);if(n<0&&errno==EINTR)continue;need(n>0,"Cannot copy original GPT proof");done+=size_t(n);}need(!fsync(file.get()),"Cannot synchronize original GPT proof");
    }need(!fsync(out.fd()),"Cannot synchronize original GPT directory");ure::gpt_backup_verify(destination);
}
std::string filesystem(bool userdata,const std::string& selected="ext4") {
    std::vector<std::string> choices;
    for(const auto& cap:native_capabilities()) {
        const auto type=cap["filesystem"].asString();if(cap["available"]!=true)continue;
        if(userdata&&type!="ext4"&&type!="f2fs"&&type!="btrfs"&&type!="xfs")continue;
        choices.push_back(type);choices.push_back(type==selected?"Current filesystem":cap.get("limitation","Native formatter and checker").asString());
    }need(!choices.empty(),"No reviewed native filesystem tools are installed");return menu("Filesystem for this volume",choices);
}
Value layout_request(bool single,const std::string& fs) {
    Value request;request["schema"]=1;request["format"]="uke-multiboot-request";request["action"]=single?"restore-default":"setup";request["keep_userdata"]=true;request["advanced"]=false;request["userdata_filesystem"]=fs;request["partitions"]=Value(Json::arrayValue);
    if(single)return request;
    const auto roles=dialog({"--separate-output","--checklist","Select volumes. Userdata keeps the remaining space (at least 64 GiB); every new volume follows userdata.","23","92","10","esp","EFI system partition (512 MiB - 4 GiB)","on","linux_boot","Separate Linux boot (at least 512 MiB)","on","linux","Linux root (at least 40 GiB)","on","linux_swap","Linux swap (at least 1 GiB)","off","windows","Windows root / NTFS (at least 50 GiB)","off","shared","Shared data","off","linux2","Second Linux root (at least 40 GiB)","off"});
    request["advanced"]=menu("Layout options",{"standard","Standard labels and order","advanced","Custom labels and order; userdata always first"})=="advanced";
    std::istringstream selected(roles);std::string role;
    while(std::getline(selected,role)) {
        if(role.empty())continue;
        need(ure::identifier(role),"Invalid role selection");Value row;row["role"]=role;
        row["size"]=input("Size for "+role+" (numeric amount)",role=="esp"||role=="linux_boot"?"1":role=="linux_swap"?"8":role=="windows"?"50":"40");
        row["unit"]=menu("Unit for "+role,{"GiB","Binary gibibytes","GB","Decimal gigabytes","MiB","Binary mebibytes","MB","Decimal megabytes","%","Percent of original userdata pool"});
        row["filesystem"]=role=="esp"?"fat32":role=="windows"?"ntfs":role=="linux_swap"?"linux-swap":filesystem(false);
        if(request["advanced"]==true)row["label"]=input("GPT label for "+role+" (unique ASCII identifier, at most 36 characters)",role);
        request["partitions"].append(row);
    }
    if(request["advanced"]==true) {
        std::string initial="userdata";for(const auto& row:request["partitions"])initial+=" "+row["role"].asString();
        const auto order=input("Partition ROLE order, separated by spaces. Userdata must be first.",initial);std::istringstream stream(order);request["order"]=Value(Json::arrayValue);while(stream>>role)request["order"].append(role);
    }return request;
}
std::string finish_instructions(const Value& plan) {
    std::string target="userdata";for(const auto& row:plan["new_rows"])if(row["role"]=="linux"){target=row["label"].asString();break;}
    return "Storage conversion does not install an operating system.\n\nIn fastboot, flash the matching system image to: "+target+"\nCheck the matching boot image's root partition contract before booting. This manager never writes boot_a, boot_b, firmware, misc or other OEM partitions.\n\nKeep the external journal and an independent backup of files. A GPT metadata backup cannot restore erased files.";
}
}
int wizard(bool singleboot,bool dry_run) {
    try {
        need(isatty(STDIN_FILENO)&&isatty(STDOUT_FILENO),"An interactive terminal is required; no unattended erase option exists");
        auto current=inspect();ure::Root system("/");power();native_tool("dialog");native_tool("dmsetup");native_tool("wipefs");
        notice("Only userdata and proven project-created children inside its ORIGINAL extent may change. Other OEM records and payloads remain protected.\n\nBack up your files first. Metadata receipts are not file backups.\n\nExisting carved layouts require the original same-device GPT archive AND a completed creation receipt. Unknown ownership, corrupt GPT or wrong-device evidence cannot be overridden.");
        const fs::path parent=input("Existing root-owned private directory on a separate ext4/XFS USB or other external disk. No part of the affected LUN may hold the journal.");trusted(parent);ure::private_directory(parent,false);journal_identity(parent,current.target);
        const auto job=parent/("uke-layout-"+ure::operation_id());ure::private_directory(job,true);
        const fs::path original=input("Original same-device GPT backup directory. Leave blank ONLY if userdata still has its original uncarved GPT layout.");std::string firmware=profile();
        if(original.empty()) {ure::multiboot_inspect(current.target,{},firmware,&system);backup(current.target,job/"original-gpt",firmware);}
        else {trusted(original);firmware=load_private(original/"manifest.json")["firmware_profile"].asString();need(ure::identifier(firmware),"Invalid original firmware profile");copy_original(original,job/"original-gpt");}
        const auto inspection=ure::multiboot_inspect(current.target,job/"original-gpt",firmware,&system);Value receipt;
        if(inspection["existing_multiboot"]==true){const fs::path path=input("Completed uke-control ownership.json receipt for the current child partitions");trusted(path.parent_path());receipt=load_private(path);require_ownership(inspection,receipt,current.target.identity);save_private(job/"previous-ownership.json",receipt);}
        const bool preserve=menu("Userdata file policy",{"preserve","Preserve the verified filesystem and files","recreate","Erase userdata and create a new Linux filesystem; two approvals"})=="preserve";
        const auto selectedfs=preserve?current.evidence.filesystem:filesystem(true,current.evidence.filesystem);
        auto request=layout_request(singleboot,selectedfs);
        const bool finish_fastboot=menu("After successful conversion",{"fastboot","Enter fastboot and flash the matching system image","stay","Stay in Linux (only if the running root remains valid)"})=="fastboot";
        auto proposal=propose(system,current.target,request,job/"original-gpt",firmware,receipt,current.evidence,preserve,finish_fastboot);
        save_private(job/"plan.json",proposal.document);const auto preview_text=preview(proposal.document);std::cout<<preview_text;
        if(dry_run){notice(preview_text+"\nRead-only preview. No storage write or reboot was prepared.");return 0;}
        if(proposal.document["route"]=="blocked"){notice(preview_text+"\nNo storage change was prepared. To choose erase/recreate, start a fresh wizard and give two explicit approvals. Filesystem failures never select erase automatically.");return 1;}
        if(proposal.document["no_changes_required"]==true){notice("The requested layout and userdata size already match. No write or reboot is needed.");return 0;}
        if(proposal.document["route"]=="shutdown")need(finish_fastboot,"Offline conversion finishes in fastboot; select fastboot in a fresh wizard");
        need(!finish_fastboot||fastboot_available(),"Bound bootloader restart support is unavailable");
        notice(preview_text);if(!yes("Approve this exact layout?\n\n"+preview_text))throw Cancel{};
        unsigned approvals=1;
        if(proposal.document["data_loss"]==true) {
            const auto phrase="ERASE "+proposal.document["sha256"].asString().substr(0,12);
            need(input("FINAL DATA LOSS APPROVAL\n\nExisting project child volumes will be erased. Userdata preservation follows the reviewed plan.\n\nType exactly: "+phrase)==phrase,"Second approval did not match; nothing was prepared");++approvals;
        }
        notice(finish_instructions(proposal.document));Value consent;consent["schema"]="org.uke.control.layout.consent.v1";consent["plan_sha256"]=proposal.document["sha256"];consent["approvals"]=approvals;consent["boot_id_sha256"]=ure::sha256(read("/proc/sys/kernel/random/boot_id"));consent["expires_ms"]=Json::UInt64(ure::monotonic_ms()+10*60*1000);consent["journal"]=journal_identity(job,current.target);consent["sha256"]=seal(consent);save_private(job/"consent.json",consent);
        power();const bool shutdown=proposal.document["route"]=="shutdown";
        const auto unit="uke-layout-"+proposal.document["sha256"].asString().substr(0,12);
        const auto result=run({native_tool("systemd-run"),"--unit="+unit,"--collect","--wait","--property=Type=exec","--property=UMask=0077","--property=NoNewPrivileges=yes","--property=SendSIGKILL=no","--property=KillMode=process","--property=TimeoutStopSec=infinity","--property=OOMScoreAdjust=-1000","--",native_tool("systemd-inhibit"),"--what=shutdown:sleep","--mode=block","--who=uke-control","--why=Complete confirmed userdata layout operation","/usr/libexec/uke-control/layout-manager",shutdown?"--prepare":"--worker",(job/"consent.json").string()});
        std::cout<<result.output;need(result.status==0,"Layout worker failed. Inspect the external journal; do not retry an already-started job");
        if(shutdown) {
            try{checked({native_tool("systemctl"),"--check-inhibitors=yes","reboot"});}catch(...){withdraw_shutdown(job);throw;}
        }else {
            const auto state=load_private(job/"state.json");need(state["completed"]==true,"Worker did not record verified completion");
            std::cout<<finish_instructions(proposal.document)<<"\nJournal: "<<job<<'\n';
            if(finish_fastboot)checked({native_tool("systemctl"),"--check-inhibitors=yes","reboot","bootloader"});
        }return 0;
    }catch(const Cancel&){std::cout<<"Cancelled. No new storage operation was authorized.\n";return 0;}
}
int command(int argc,char** argv) {
    need(argc>=2,"Expected singleboot or multiboot");const std::string action=argv[1];
    if(action=="--offline"){need(argc==2,"Unexpected offline argument");return offline();}
    if(action=="--worker"||action=="--prepare") {
        need(argc==3&&geteuid()==0,"A private root job request is required");const fs::path request=argv[2];
        if(action=="--prepare")prepare_shutdown(request.parent_path());else run_worker(request,false,[](const auto& phase){std::cout<<phase<<std::endl;});return 0;
    }
    need(action=="singleboot"||action=="multiboot","Unknown boot layout command");bool as_json=false,dry=false,status=false;
    for(int i=2;i<argc;++i){const std::string word=argv[i];if(word=="--json")as_json=true;else if(word=="--dry-run")dry=true;else if(word=="status")status=true;else throw cli::UsageError("expected "+action+" [status | --dry-run]");}
    if(status){need(!dry,"Status cannot be combined with an interactive preview");cli::print(ure::json(inspect().summary),as_json);return 0;}
    if(as_json)throw cli::UsageError("--json requires singleboot status or multiboot status; the wizard is interactive");
    return wizard(action=="singleboot",dry);
}
}
#ifndef UKE_LAYOUT_NO_MAIN
int main(int argc,char** argv) {
    try{return uke::layout::command(argc,argv);}catch(const uke::cli::UsageError& error){std::cerr<<"uke-control: "<<error.what()<<'\n';return 64;}catch(const std::exception& error){std::cerr<<"uke-control: "<<error.what()<<'\n';return 1;}
}
#endif
