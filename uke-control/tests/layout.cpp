// SPDX-License-Identifier: MIT
#include "layout-fixture.hpp"
#include <iostream>
int main(int argc,char** argv) {
    using namespace fixture;namespace fs=std::filesystem;
    if(argc==3&&std::string(argv[1])=="--create"){image(argv[2]);return 0;}
    char pattern[]="/tmp/uke-layout-policy-XXXXXX";const auto directory=mkdtemp(pattern);assert(directory);const fs::path work=directory;
    try {
        if(fs::exists("/sys/class/dmi/id/sys_vendor"))assert(!read("/sys/class/dmi/id/sys_vendor").empty());
        assert(!read("/proc/self/mountinfo",4*1024*1024).empty());
        const auto fsimage=work/"ext4";ure::Fd fsfd(open(fsimage.c_str(),O_RDWR|O_CREAT|O_EXCL|O_CLOEXEC,0600));assert(fsfd.get()>=0&&!ftruncate(fsfd.get(),64*mib));
        checked({native_tool("mkfs.ext4"),"-q","-F","-E","nodiscard",fsimage});const auto native=filesystem_probe(fsfd.get(),0,64*mib);
        assert(native["type"]=="ext4"&&native["size_verified"]==true&&native["filesystem_bytes"]==Json::UInt64(64*mib)&&ure::uuid(native["uuid"].asString()));
        const auto path=work/"disk";image(path);auto disk=ure::storage_image(path,4096);ure::Root system("/");assert(ure::gpt_inspect(disk.descriptor.get(),4096)["healthy"]==true);
        const auto saved=backup(disk,work/"original","fixture");assert(ure::gpt_backup_verify(work/"original")["verified"]==true);
        Evidence ev;ev.userdata_index=30;ev.root_index=30;ev.userdata_mount="/";ev.mounted.insert(30);ev.filesystem="ext4";ev.filesystem_uuid="fixture-fs";ev.partition_bytes=250*gib-64*mib;ev.filesystem_bytes=ev.partition_bytes;ev.filesystem_size_verified=true;ev.single_device=true;
        auto r=request();add(r,"esp","512","MiB","fat32");add(r,"linux_boot","512","MiB","ext4");add(r,"linux","40","GiB","ext4");
        auto p=propose(system,disk,r,work/"original","fixture",{},ev,true,true);assert(p.document["route"]=="shutdown");assert(p.document["confirmations_required"]==1);assert(p.document["protected_records"].size()==31);
        assert(p.document["new_rows"][0]["role"]=="userdata");std::uint64_t end=64*mib;
        for(auto& row:p.document["new_rows"]){assert(row["offset"]==Json::UInt64(end));end+=row["bytes"].asUInt64();assert(end<=250*gib);}assert(p.document["sha256"]==seal(p.document));
        for(auto& range:p.after)if(range.name=="primary_header")assert(static_cast<unsigned char>(range.bytes[80])==64);
        auto bad=p.after;bad[0].bytes[0]^=1;refuse([&]{validate_protected(disk,ure::multiboot_inspect(disk,work/"original","fixture",&system),p.before,bad);});
        auto changed=r;changed["advanced"]=true;changed["order"]=Value(Json::arrayValue);for(auto* role:{"esp","userdata","linux_boot","linux"})changed["order"].append(role);refuse([&]{propose(system,disk,changed,work/"original","fixture",{},ev,true,true);});
        changed=r;changed["partitions"][0]["size"]="5";changed["partitions"][0]["unit"]="GiB";refuse([&]{propose(system,disk,changed,work/"original","fixture",{},ev,true,true);});
        changed=r;changed["partitions"][2]["size"]="500";refuse([&]{propose(system,disk,changed,work/"original","fixture",{},ev,true,true);});
        auto online=ev;online.filesystem="btrfs";changed=r;changed["userdata_filesystem"]="btrfs";auto btrfs=propose(system,disk,changed,work/"original","fixture",{},online,true,false);assert(btrfs.document["route"]=="direct");assert(btrfs.document["new_rows"][0]["filesystem"]=="btrfs");
        online.single_device=false;assert(propose(system,disk,changed,work/"original","fixture",{},online,true,false).document["route"]=="blocked");online=ev;online.filesystem="f2fs";changed=r;changed["userdata_filesystem"]="f2fs";assert(propose(system,disk,changed,work/"original","fixture",{},online,true,true).document["route"]=="direct");
        online.filesystem="xfs";changed["userdata_filesystem"]="xfs";assert(propose(system,disk,changed,work/"original","fixture",{},online,true,true).document["route"]=="blocked");online.filesystem_bytes=100*gib;assert(propose(system,disk,changed,work/"original","fixture",{},online,true,false).document["route"]=="direct");
        auto erased=propose(system,disk,r,work/"original","fixture",{},ev,false,true);assert(erased.document["confirmations_required"]==2&&erased.document["root_partition_removed"]==true&&erased.document["route"]=="shutdown");assert(propose(system,disk,r,work/"original","fixture",{},ev,false,false).document["route"]=="blocked");
        ure::Fd output(open(path.c_str(),O_RDWR|O_CLOEXEC));for(const auto& range:p.after)write(output.get(),range.bytes,range.offset);assert(!fsync(output.get()));disk=ure::storage_image(path,4096);
        auto proof=receipt(p.document,disk.identity);require_ownership(ure::multiboot_inspect(disk,work/"original","fixture",&system),proof,disk.identity);
        auto broken=proof;broken["owned_rows"][0]["end_lba"]=Json::UInt64(1);broken["sha256"]=seal(broken);refuse([&]{require_ownership(ure::multiboot_inspect(disk,work/"original","fixture",&system),broken,disk.identity);});
        auto single=request(true);ev.partition_bytes=p.document["new_rows"][0]["bytes"].asUInt64();ev.filesystem_bytes=ev.partition_bytes;ev.root_index=p.document["new_rows"][3]["index"].asUInt();ev.mounted.clear();ev.mounted.insert(ev.root_index);ev.userdata_mount="";
        auto restore=propose(system,disk,single,work/"original","fixture",proof,ev,true,true);assert(restore.document["new_rows"].size()==1&&restore.document["route"]=="shutdown"&&restore.document["confirmations_required"]==2);assert(restore.document["new_rows"][0]["bytes"]==Json::UInt64(250*gib-64*mib));
        refuse([&]{propose(system,disk,single,work/"original","fixture",{},ev,true,true);});assert(ure::storage_read(disk.descriptor.get(),mib,27)=="OEM PAYLOAD MUST NOT CHANGE");
        save_private(work/"record.json",proof);assert(ure::json(load_private(work/"record.json"))==ure::json(proof));assert(!chmod((work/"record.json").c_str(),0644));refuse([&]{load_private(work/"record.json");});
        fs::remove_all(work);std::cout<<"Layout policy: allocation, OEM bytes, table expansion, live/offline routes, erase approvals, original proof, exact ownership and private records passed\n";
    }catch(...){fs::remove_all(work);throw;}
}
