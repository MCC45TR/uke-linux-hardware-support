// SPDX-License-Identifier: MIT
#include "layout-fixture.hpp"
#include "../src/watchdog-state.hpp"
#include <fstream>
#include <iostream>
int main(int argc,char** argv) {
    using namespace fixture;namespace fs=std::filesystem;
    if(argc==3&&std::string(argv[1])=="--create"){image(argv[2]);return 0;}
    char pattern[]="/tmp/uke-layout-policy-XXXXXX";const auto directory=mkdtemp(pattern);assert(directory);const fs::path work=directory;
    try {
        const auto watchdog=work/"watchdogs";fs::create_directory(watchdog);uke::require_inactive_watchdogs(watchdog);
        fs::create_directory(watchdog/"watchdog0");refuse([&]{uke::require_inactive_watchdogs(watchdog);});
        for(const auto* state:{"active\n","unknown\n","inactive extra\n"}){std::ofstream(watchdog/"watchdog0/state")<<state;refuse([&]{uke::require_inactive_watchdogs(watchdog);});}
        std::ofstream(watchdog/"watchdog0/state")<<"inactive\n";uke::require_inactive_watchdogs(watchdog);
        if(fs::exists("/sys/class/dmi/id/sys_vendor"))assert(!read("/sys/class/dmi/id/sys_vendor").empty());
        assert(!read("/proc/self/mountinfo",4*1024*1024).empty());
        const auto fsimage=work/"ext4";ure::Fd fsfd(open(fsimage.c_str(),O_RDWR|O_CREAT|O_EXCL|O_CLOEXEC,0600));assert(fsfd.get()>=0&&!ftruncate(fsfd.get(),64*mib));
        const auto resize_help=run({native_tool("resize.f2fs"),"-h"});assert(resize_help.output.find("Usage: resize.f2fs")!=std::string::npos);
        checked({native_tool("mkfs.ext4"),"-q","-F","-E","nodiscard",fsimage});const auto native=filesystem_probe(fsfd.get(),0,64*mib);
        assert(native["type"]=="ext4"&&native["size_verified"]==true&&native["filesystem_bytes"]==Json::UInt64(64*mib)&&ure::uuid(native["uuid"].asString()));
        const auto journal_inode=ure::storage_read(fsfd.get(),1024+224,4);write(fsfd.get(),std::string(4,'\0'),1024+224);assert(!fsync(fsfd.get()));
        assert(filesystem_probe(fsfd.get(),0,64*mib)["external_journal"]==true);write(fsfd.get(),journal_inode,1024+224);assert(!fsync(fsfd.get()));
        const auto path=work/"disk";image(path);auto disk=ure::storage_image(path,4096);ure::Root system("/");assert(ure::gpt_inspect(disk.descriptor.get(),4096)["healthy"]==true);
        const auto saved=backup(disk,work/"original","fixture");assert(ure::gpt_backup_verify(work/"original")["verified"]==true);
        Evidence ev;ev.userdata_index=30;ev.root_index=30;ev.userdata_mount="/";ev.mounted.insert(30);ev.filesystem="ext4";ev.filesystem_uuid="fixture-fs";ev.partition_bytes=250*gib-64*mib;ev.filesystem_bytes=ev.partition_bytes;ev.filesystem_size_verified=true;ev.single_device=true;
        auto r=request();add(r,"esp","512","MiB","fat32");add(r,"linux_boot","512","MiB","ext4");add(r,"linux","40","GiB","ext4");
        auto p=propose(system,disk,r,work/"original","fixture",{},ev,true,true);assert(p.document["route"]=="shutdown");assert(p.document["confirmations_required"]==1);assert(p.document["protected_records"].size()==31);
        assert(p.document["new_rows"][0]["role"]=="userdata");std::uint64_t end=64*mib;
        for(auto& row:p.document["new_rows"]){assert(row["offset"]==Json::UInt64(end));end+=row["bytes"].asUInt64();assert(end<=250*gib);}assert(p.document["sha256"]==seal(p.document));
        for(auto& range:p.after)if(range.name=="primary_header")assert(static_cast<unsigned char>(range.bytes[80])==64);
        snapshot(disk,p.before,work/"before-gpt","fixture","observed-before",p.document["sha256"].asString());
        snapshot(disk,p.after,work/"after-gpt","fixture","planned-after",p.document["sha256"].asString());
        for(const auto* name:{"before-gpt","after-gpt"})assert(ure::gpt_backup_verify(work/name)["verified"]==true);
        const auto snapshot_file=work/"after-gpt/snapshot.json";
        assert(ure::json_file(snapshot_file)["kind"]=="planned-after");
        if(geteuid()==0)assert(load_private(snapshot_file)["kind"]=="planned-after");
        else refuse([&]{load_private(snapshot_file);});
        auto consent_for=[](const Value& plan){Value c;c["schema"]="org.uke.control.layout.consent.v1";c["plan_sha256"]=plan["sha256"];c["approvals"]=plan["confirmations_required"];c["boot_id_sha256"]="fixture-boot";c["expires_ms"]=Json::UInt64(2000);c["sha256"]=seal(c);return c;};
        const auto consent=consent_for(p.document);const auto pinned=consent["sha256"].asString();validate_authorization(p.document,consent,pinned,"fixture-boot",1000);
        auto altered=consent;altered["expires_ms"]=Json::UInt64(3000);altered["sha256"]=seal(altered);refuse([&]{validate_authorization(p.document,altered,pinned,"fixture-boot",1000);});
        auto altered_plan=p.document;altered_plan["finish"]="stay";altered_plan["sha256"]=seal(altered_plan);altered=consent_for(altered_plan);refuse([&]{validate_authorization(altered_plan,altered,pinned,"fixture-boot",1000);});
        altered=consent;altered["approvals"]=0;altered["sha256"]=seal(altered);refuse([&]{validate_authorization(p.document,altered,altered["sha256"].asString(),"fixture-boot",1000);});
        refuse([&]{validate_authorization(p.document,consent,pinned,"other-boot",1000);});refuse([&]{validate_authorization(p.document,consent,pinned,"fixture-boot",2001);});
        altered=consent;altered["expires_ms"]=Json::UInt64(602000);altered["sha256"]=seal(altered);refuse([&]{validate_authorization(p.document,altered,altered["sha256"].asString(),"fixture-boot",1000);});
        auto flash=p.document;flash["new_rows"]=Value(Json::arrayValue);Value linux2;linux2["role"]="linux2";linux2["label"]="next_root";flash["new_rows"].append(linux2);
        assert(finish_instructions(flash).find("to: next_root")!=std::string::npos);flash["new_rows"]=Value(Json::arrayValue);assert(finish_instructions(flash).find("No Linux root was selected")!=std::string::npos);
        flash["request"]["action"]="restore-default";assert(finish_instructions(flash).find("to: userdata")!=std::string::npos);
        auto no_op=propose(system,disk,request(true),work/"original","fixture",{},ev,true,true);assert(no_op.document["no_changes_required"]==true&&no_op.document["confirmations_required"]==0);
        auto bad=p.after;bad[0].bytes[0]^=1;refuse([&]{validate_protected(disk,ure::multiboot_inspect(disk,work/"original","fixture",&system),p.before,bad);});
        auto changed=r;changed["advanced"]=true;changed["order"]=Value(Json::arrayValue);for(auto* role:{"esp","userdata","linux_boot","linux"})changed["order"].append(role);refuse([&]{propose(system,disk,changed,work/"original","fixture",{},ev,true,true);});
        changed=r;changed["partitions"][0]["size"]="5";changed["partitions"][0]["unit"]="GiB";refuse([&]{propose(system,disk,changed,work/"original","fixture",{},ev,true,true);});
        changed=r;changed["partitions"][2]["size"]="500";refuse([&]{propose(system,disk,changed,work/"original","fixture",{},ev,true,true);});
        auto online=ev;online.filesystem="btrfs";changed=r;changed["userdata_filesystem"]="btrfs";auto btrfs=propose(system,disk,changed,work/"original","fixture",{},online,true,false);assert(btrfs.document["route"]=="direct");assert(btrfs.document["new_rows"][0]["filesystem"]=="btrfs");
        online.single_device=false;assert(propose(system,disk,changed,work/"original","fixture",{},online,true,false).document["route"]=="blocked");online=ev;online.filesystem="f2fs";changed=r;changed["userdata_filesystem"]="f2fs";assert(propose(system,disk,changed,work/"original","fixture",{},online,true,true).document["route"]=="blocked");
        auto f2fs_erase=propose(system,disk,changed,work/"original","fixture",{},online,false,true);assert(f2fs_erase.document["route"]=="shutdown"&&f2fs_erase.document["confirmations_required"]==2);
        online.filesystem_bytes=100*gib;assert(propose(system,disk,changed,work/"original","fixture",{},online,true,true).document["route"]=="shutdown");online.filesystem_bytes=ev.filesystem_bytes;
        online.single_device=false;assert(propose(system,disk,changed,work/"original","fixture",{},online,false,true).document["route"]=="blocked");online.single_device=true;online.external_journal=true;assert(propose(system,disk,changed,work/"original","fixture",{},online,false,true).document["route"]=="blocked");online.external_journal=false;
        online.filesystem="xfs";changed["userdata_filesystem"]="xfs";assert(propose(system,disk,changed,work/"original","fixture",{},online,true,true).document["route"]=="blocked");online.filesystem_bytes=100*gib;assert(propose(system,disk,changed,work/"original","fixture",{},online,true,false).document["route"]=="direct");
        auto erased=propose(system,disk,r,work/"original","fixture",{},ev,false,true);assert(erased.document["confirmations_required"]==2&&erased.document["root_partition_removed"]==true&&erased.document["route"]=="shutdown");assert(propose(system,disk,r,work/"original","fixture",{},ev,false,false).document["route"]=="blocked");
        ure::Fd output(open(path.c_str(),O_RDWR|O_CLOEXEC));for(const auto& range:p.after)write(output.get(),range.bytes,range.offset);assert(!fsync(output.get()));disk=ure::storage_image(path,4096);
        StorageTarget imported_disk{ure::Fd(dup(disk.descriptor.get())),disk.identity};
        Value legacy;legacy["schema"]=1;legacy["operation"]="dualboot.setup";legacy["firmware_profile"]="fixture";legacy["target_identity"]=imported_disk.identity;
        legacy["gpt"]["target_identity"]=imported_disk.identity;legacy["gpt"]["before"]=p.document["before"];legacy["gpt"]["after"]=p.document["after"];
        legacy["gpt"]["plan_sha256"]=seal(legacy["gpt"],"plan_sha256");legacy["plan_sha256"]=seal(legacy,"plan_sha256");
        Value terminal;terminal["schema"]=1;terminal["format"]="ure-live-dualboot-journal";terminal["phase"]="COMMITTED_REBOOT_REQUIRED";terminal["plan_sha256"]=legacy["plan_sha256"];
        Value checkpoint;checkpoint["schema"]=1;checkpoint["format"]="ure-adb-dualboot-checkpoint";checkpoint["phase"]=terminal["phase"];checkpoint["plan_sha256"]=legacy["plan_sha256"];checkpoint["session"]="fixture-session";checkpoint["sequence"]=Json::UInt64(13);checkpoint["files"]=Value(Json::arrayValue);
        auto member=[&](const std::string& name,const std::string& bytes){Value row;row["name"]=name;row["sha256"]=ure::sha256(bytes);std::string hex;static constexpr char digits[]="0123456789abcdef";for(unsigned char c:bytes){hex+=digits[c>>4];hex+=digits[c&15];}row["hex"]=hex;checkpoint["files"].append(row);};
        member("plan.json",ure::json(legacy));member("state.json",ure::json(terminal));
        for(const auto& range:p.before)member("before-"+range.name+".bin",range.bytes);
        for(const auto& range:p.after)member("after-"+range.name+".bin",range.bytes);
        auto invalid=checkpoint;invalid["phase"]="PREPARED";refuse([&]{import_recovery_checkpoint(system,imported_disk,invalid,work/"invalid-import");});
        invalid=checkpoint;invalid["files"][2]["sha256"]=ure::sha256("changed");refuse([&]{import_recovery_checkpoint(system,imported_disk,invalid,work/"invalid-import");});
        auto foreign=imported_disk.identity;imported_disk.identity["bytes"]=Json::UInt64(1);refuse([&]{import_recovery_checkpoint(system,imported_disk,checkpoint,work/"invalid-import");});imported_disk.identity=foreign;
        const auto imported=import_recovery_checkpoint(system,imported_disk,checkpoint,work/"imported");assert(imported["completed"]==true&&imported["owned_rows"].size()==3);
        refuse([&]{import_recovery_checkpoint(system,imported_disk,checkpoint,work/"imported");});
        for(const auto& range:p.after)assert(ure::storage_read(disk.descriptor.get(),range.offset,range.bytes.size())==range.bytes);
        auto switched=p.after;
        auto ranges_crc=[](std::vector<StorageRange>& ranges) {
            auto find=[&](const char* name)->std::string&{for(auto& range:ranges)if(range.name==name)return range.bytes;throw std::runtime_error("Missing fixture metadata");};
            auto number=[](const std::string& bytes,size_t at){unsigned value=0;for(unsigned i=0;i<4;++i)value|=unsigned(static_cast<unsigned char>(bytes[at+i]))<<(8*i);return value;};
            for(const auto& [h,t]:{std::pair{"primary_header","primary_table"},std::pair{"backup_header","backup_table"}}){auto& header=find(h);const auto& table=find(t);put(header,88,crc(std::string_view(table).substr(0,size_t(number(header,80))*number(header,84))),4);put(header,16,0,4);put(header,16,crc(std::string_view(header).substr(0,number(header,12))),4);}
        };
        auto apply_fixture=[&](const std::vector<StorageRange>& ranges){for(const auto& range:ranges)write(output.get(),range.bytes,range.offset);assert(!fsync(output.get()));imported_disk=ure::storage_image(path,4096);};
        for(auto& range:switched)if(range.name=="primary_table"||range.name=="backup_table") {
            auto& table=range.bytes;const auto a=table.substr(9*128,16),b=table.substr(10*128,16);table.replace(9*128,16,b);table.replace(10*128,16,a);table[9*128+54]^=4;table[10*128+54]^=4;
        }
        ranges_crc(switched);apply_fixture(switched);
        const auto slot_receipt=import_recovery_checkpoint(system,imported_disk,checkpoint,work/"imported-slots");assert(slot_receipt["preserved_oem_slot_transitions"].size()==1);
        assert(ure::gpt_backup_verify(work/"imported-slots/recovery-original-gpt")["verified"]==true);
        auto restored=propose(system,imported_disk,request(true),work/"imported-slots/original-gpt","fixture",slot_receipt,ev,true,true);assert(restored.document["new_rows"].size()==1);
        auto unexpected=switched;for(auto& range:unexpected)if(range.name=="primary_table"||range.name=="backup_table")range.bytes[0]^=1;
        ranges_crc(unexpected);apply_fixture(unexpected);refuse([&]{import_recovery_checkpoint(system,imported_disk,checkpoint,work/"invalid-oem-import");});
        auto broken_pair=switched;for(auto& range:broken_pair)if(range.name=="primary_table"||range.name=="backup_table")range.bytes[9*128+54]^=8;
        ranges_crc(broken_pair);apply_fixture(broken_pair);refuse([&]{import_recovery_checkpoint(system,imported_disk,checkpoint,work/"invalid-slot-import");});
        apply_fixture(p.after);disk=ure::storage_image(path,4096);
        auto proof=receipt(p.document,disk.identity);require_ownership(ure::multiboot_inspect(disk,work/"original","fixture",&system),proof,disk.identity);
        auto broken=proof;broken["owned_rows"][0]["end_lba"]=Json::UInt64(1);broken["sha256"]=seal(broken);refuse([&]{require_ownership(ure::multiboot_inspect(disk,work/"original","fixture",&system),broken,disk.identity);});
        auto single=request(true);ev.partition_bytes=p.document["new_rows"][0]["bytes"].asUInt64();ev.filesystem_bytes=ev.partition_bytes;ev.root_index=p.document["new_rows"][3]["index"].asUInt();ev.mounted.clear();ev.mounted.insert(ev.root_index);ev.userdata_mount="";
        auto restore=propose(system,disk,single,work/"original","fixture",proof,ev,true,true);assert(restore.document["new_rows"].size()==1&&restore.document["route"]=="shutdown"&&restore.document["confirmations_required"]==2);assert(restore.document["new_rows"][0]["bytes"]==Json::UInt64(250*gib-64*mib));
        auto mismatched=propose(system,disk,r,work/"original","fixture",proof,ev,true,true);assert(mismatched.document["no_changes_required"]==false&&mismatched.document["confirmations_required"]==2);
        refuse([&]{propose(system,disk,single,work/"original","fixture",{},ev,true,true);});assert(ure::storage_read(disk.descriptor.get(),mib,27)=="OEM PAYLOAD MUST NOT CHANGE");
        const auto record_file=work/"record.json";save_private(record_file,proof);
        assert(ure::json(ure::json_file(record_file))==ure::json(proof));
        if(geteuid()==0)assert(ure::json(load_private(record_file))==ure::json(proof));
        else refuse([&]{load_private(record_file);});
        assert(!chmod(record_file.c_str(),0644));refuse([&]{load_private(record_file);});
        std::cout<<(geteuid()==0?"Root-owned private-record round trip passed\n":"Unprivileged private-record ownership rejection passed\n");
        fs::remove_all(work);std::cout<<"Layout policy: allocation, OEM bytes, table expansion, live/offline routes, erase approvals, original proof, exact ownership and private records passed\n";
    }catch(...){fs::remove_all(work);throw;}
}
