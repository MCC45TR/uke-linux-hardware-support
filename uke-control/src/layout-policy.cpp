// SPDX-License-Identifier: MIT
#include "layout.hpp"
#include "../shared/userdata-grow.hpp"
#include <algorithm>
#include <array>
#include <blkid/blkid.h>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <linux/memfd.h>
#include <linux/btrfs_tree.h>
#include <memory>
#include <sstream>
#include <sys/mman.h>
#include <unistd.h>
namespace uke::layout {
namespace {
void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Value descriptions(const std::vector<StorageRange>& ranges) {
    Value out(Json::arrayValue);
    for(const auto& r:ranges){Value row;row["name"]=r.name;row["offset"]=Json::UInt64(r.offset);row["bytes"]=Json::UInt64(r.bytes.size());row["sha256"]=ure::sha256(r.bytes);out.append(row);}return out;
}
void write(int fd,std::string_view bytes,std::uint64_t offset=0) {
    while(!bytes.empty()) {const auto n=pwrite(fd,bytes.data(),bytes.size(),static_cast<off_t>(offset));if(n<0&&errno==EINTR)continue;need(n>0,"Cannot write private metadata snapshot");offset+=n;bytes.remove_prefix(n);}
}
const StorageRange& named(const std::vector<StorageRange>& ranges,const std::string& name) {
    auto it=std::find_if(ranges.begin(),ranges.end(),[&](const auto& r){return r.name==name;});need(it!=ranges.end(),"Missing GPT metadata range");return *it;
}
Value row_key(const Value& row) {
    Value out;for(const auto* key:{"index","label","partuuid","type_guid","start_lba","end_lba","attributes"})out[key]=row[key];return out;
}
bool native(const std::string& path) {
    try{native_tool(std::filesystem::path(path).filename().string());return true;}
    catch(const std::exception&){return false;}
}
std::string bytes(std::uint64_t value){std::ostringstream out;out<<std::fixed<<std::setprecision(2)<<static_cast<double>(value)/1073741824.0<<" GiB";return out.str();}
}
std::string seal(Value value,const char* field){value.removeMember(field);return ure::sha256(ure::json(value));}
Value filesystem_probe(int fd,std::uint64_t offset,std::uint64_t size) {
    // Filesystem tools use partition/DM mappings; the parent LUN may cache an older superblock.
    need(offset<=INT64_MAX&&size<=INT64_MAX&&posix_fadvise(fd,offset,size,POSIX_FADV_DONTNEED)==0,"Cannot refresh bounded filesystem observations");
    auto result=ure::filesystem_probe_range(fd,offset,size);
    std::unique_ptr<blkid_struct_probe,decltype(&blkid_free_probe)> probe(blkid_new_probe(),blkid_free_probe);
    need(bool(probe)&&offset<=INT64_MAX&&size<=INT64_MAX,"Cannot initialize bounded filesystem probe");
    need(blkid_probe_set_device(probe.get(),fd,static_cast<blkid_loff_t>(offset),static_cast<blkid_loff_t>(size))==0,"Cannot bound filesystem probe");
    blkid_probe_enable_partitions(probe.get(),0);
    blkid_probe_set_superblocks_flags(probe.get(),BLKID_SUBLKS_TYPE|BLKID_SUBLKS_UUID|BLKID_SUBLKS_FSINFO);
    result["size_verified"]=false;
    if(blkid_do_safeprobe(probe.get())!=0)return result;
    auto field=[&](const char* key){const char* value=nullptr;size_t length=0;if(blkid_probe_lookup_value(probe.get(),key,&value,&length))return std::string();if(length&&value[length-1]=='\0')--length;std::string text(value,length);need(text.find('\0')==text.npos,"Embedded NUL in filesystem observation");return text;};
    auto type=field("TYPE");if(!type.empty())result["type"]=type;
    auto uuid=field("UUID");if(!uuid.empty())result["uuid"]=uuid;
    result["external_journal"]=!field("EXT_JOURNAL").empty();
    const auto blocks=field("FSLASTBLOCK"),block=field("FSBLOCKSIZE");
    if(!blocks.empty()&&!block.empty()) {
        auto decimal=[](const std::string& s){need(s.size()<=20&&s.find_first_not_of("0123456789")==s.npos,"Invalid filesystem geometry");return std::stoull(s);};
        const auto count=decimal(blocks),unit=decimal(block);
        need(unit>0&&count<=size/unit,"Filesystem exceeds its admitted partition");
        result["filesystem_bytes"]=Json::UInt64(count*unit);result["size_verified"]=true;
    }
    if(result["type"]=="f2fs") {
        userdata::Header a{},b{};
        if(pread(fd,a.data(),a.size(),static_cast<off_t>(offset+1024))==ssize_t(a.size())&&
           pread(fd,b.data(),b.size(),static_cast<off_t>(offset+5120))==ssize_t(b.size())) {
            try{result["filesystem_bytes"]=Json::UInt64(userdata::filesystem_bytes(a,b,size));result["size_verified"]=true;}
            catch(const std::exception&){result["size_verified"]=false;}
        }
    }
    auto raw=[&](std::uint64_t at,std::size_t n){need(at<=size&&n<=size-at,"Filesystem field escapes its partition");return ure::storage_read(fd,offset+at,n);};
    auto nonzero=[](const std::string& value){return std::any_of(value.begin(),value.end(),[](char c){return c!=0;});};
    if(result["type"]=="ext4")result["external_journal"]=result["external_journal"].asBool()||nonzero(raw(1024+228,4));
    if(result["type"]=="xfs")result["external_journal"]=nonzero(raw(16,8))||!nonzero(raw(48,8));
    if(result["type"]=="btrfs") {
        const auto bytes=raw(65536,sizeof(btrfs_super_block));btrfs_super_block super{};memcpy(&super,bytes.data(),bytes.size());
        result["single_device"]=super.num_devices==1;
        result["filesystem_bytes"]=Json::UInt64(super.dev_item.total_bytes);result["size_verified"]=super.num_devices==1&&super.dev_item.total_bytes>0&&super.dev_item.total_bytes<=size;
    }
    return result;
}
Value native_capabilities() {
    Value out(Json::arrayValue);
    for(const auto& [type,formatter,checker]:std::array<std::array<const char*,3>,9>{{
        {"ext4","mkfs.ext4","e2fsck"},{"f2fs","mkfs.f2fs","fsck.f2fs"},{"btrfs","mkfs.btrfs","btrfs"},
        {"fat32","mkfs.vfat","fsck.fat"},{"ntfs","mkntfs","ntfsfix"},{"exfat","mkfs.exfat","fsck.exfat"},
        {"xfs","mkfs.xfs","xfs_repair"},{"linux-swap","mkswap","blkid"},{"zfs","zpool","zpool"}}}) {
        Value row;row["filesystem"]=type;row["formatter"]="/usr/bin/"+std::string(formatter);row["checker"]="/usr/bin/"+std::string(checker);
        row["available"]=std::string_view(type)!="zfs"&&native(row["formatter"].asString())&&native(row["checker"].asString());
        if(row["available"]==true&&std::string_view(type)=="exfat") {
            const auto help=run({native_tool("mkfs.exfat"),"--help"});
            row["available"]=help.output.find("-P")!=help.output.npos&&help.output.find("none")!=help.output.npos;
        }
        row["reason"]=row["available"]==true?"":"reviewed native formatter/checker required";
        if(std::string_view(type)=="zfs")row["reason"]="No reviewed ZFS pool ownership/rollback adapter";
        if(std::string_view(type)=="ntfs")row["limitation"]="ntfsfix is a limited metadata check; Windows chkdsk remains separate";
        out.append(row);
    }return out;
}
Value backup(const StorageTarget& target,const std::filesystem::path& destination,const std::string& profile) {
    auto store=ure::private_directory(destination,true);const auto ranges=ure::gpt_regions(target.descriptor.get(),target.identity["logical_sector_bytes"].asUInt());
    const auto table=ure::gpt_inspect(target.descriptor.get(),target.identity["logical_sector_bytes"].asUInt());need(table["healthy"]==true,"Original GPT copies must be healthy and agree");
    auto save=[&](const std::string& name,const std::string& data){auto fd=store.open(name,O_RDWR|O_CREAT|O_EXCL,0600);write(fd.get(),data);need(!fsync(fd.get())&&ure::sha256(fd.get())==ure::sha256(data)&&!fsync(store.fd()),"Cannot verify durable GPT backup");};
    for(const auto& r:ranges)save(r.name+".bin",r.bytes);
    store.save_record("partition-table.json",table);
    Value manifest;manifest["schema"]=1;manifest["format"]="ure-gpt-backup";manifest["target_identity"]=target.identity;
    manifest["firmware_profile"]=profile;manifest["firmware_identity_validated"]=false;manifest["created_utc"]=ure::utc();
    manifest["tool"]="uke-control/layout-gpt-v1";manifest["regions"]=descriptions(ranges);manifest["private_record"]=true;
    manifest["partition_table_sha256"]=ure::sha256(ure::json(table));manifest["physical_test_record"]=false;
    manifest["manifest_sha256"]=seal(manifest,"manifest_sha256");store.save_record("manifest.json",manifest);
    std::string sums;for(const auto& r:ranges)sums+=ure::sha256(r.bytes)+"  "+r.name+".bin\n";
    sums+=manifest["partition_table_sha256"].asString()+"  partition-table.json\n"+ure::sha256(ure::json(manifest))+"  manifest.json\n";save("SHA256SUMS",sums);
    ure::gpt_backup_verify(destination);return manifest;
}
void require_ownership(const Value& inspection,const Value& receipt,const Value& identity) {
    if(inspection["existing_multiboot"]!=true)return;
    need(receipt["schema"]=="org.uke.control.layout.ownership.v1"&&receipt["sha256"]==seal(receipt)&&receipt["completed"]==true,
         "Existing children require a verified completed creation receipt; labels or location alone do not establish ownership");
    need(receipt["disk_guid"]==inspection["current_table"]["disk_guid"]&&receipt["original_manifest_sha256"]==inspection["original_manifest_sha256"]&&
         receipt["unit_identity_sha256"]==identity["unit_identity_sha256"]&&receipt["lun_address"]==identity["lun_address"],"Creation receipt belongs to another disk, unit or original userdata pool");
    for(const auto& row:inspection["old_rows"])if(row["label"]!="userdata") {
        unsigned matches=0;for(const auto& owned:receipt["owned_rows"])if(ure::json(row_key(owned))==ure::json(row_key(row)))++matches;
        need(matches==1,"An existing partition has no exact project ownership receipt; OEM/unknown partitions cannot be reclaimed");
    }
}
void validate_protected(const StorageTarget& target,const Value& inspection,const std::vector<StorageRange>& before,const std::vector<StorageRange>& after) {
    need(before.size()==5&&after.size()==5,"Both complete GPT copies and the unchanged protective MBR are required");
    for(const auto& r:before){const auto& next=named(after,r.name);need(next.offset==r.offset&&next.bytes.size()==r.bytes.size(),"GPT metadata locations or reservation size changed");}
    need(named(before,"protective_mbr").bytes==named(after,"protective_mbr").bytes,"Protective MBR modification is forbidden");
    for(const auto* side:{"primary_table","backup_table"})for(const auto& row:inspection["protected_records"]) {
        const auto index=row["index"].asUInt();need(index>0,"Invalid protected GPT record");const auto at=size_t(index-1)*128;
        need(at+128<=named(before,side).bytes.size()&&named(before,side).bytes.substr(at,128)==named(after,side).bytes.substr(at,128),"A protected OEM GPT record was modified");
    }
    for(const auto* side:{"primary_header","backup_header"}) {
        const auto& old=named(before,side).bytes;const auto& next=named(after,side).bytes;
        for(size_t i=0;i<old.size();++i)if(!((i>=16&&i<20)||(i>=80&&i<84)||(i>=88&&i<92)))
            need(old[i]==next[i],"A GPT header field outside checksums/entry count changed");
    }
    ure::Fd image(memfd_create("uke-layout-proposal",MFD_CLOEXEC));need(image.get()>=0&&target.identity["bytes"].asUInt64()<=INT64_MAX&&
        !ftruncate(image.get(),static_cast<off_t>(target.identity["bytes"].asUInt64())),"Cannot verify proposed sparse GPT");
    for(const auto& r:after)write(image.get(),r.bytes,r.offset);
    const auto table=ure::gpt_inspect(image.get(),target.identity["logical_sector_bytes"].asUInt());need(table["healthy"]==true,"Proposed GPT does not have two healthy matching copies");
    need(table["disk_guid"]==inspection["current_table"]["disk_guid"],"Disk GUID modification is forbidden");
    for(const auto* side:{"primary","backup"})for(const auto* field:{"first_usable_lba","last_usable_lba","entries_lba","entry_size"})
        need(table[side][field]==inspection["current_table"][side][field],"Protected GPT geometry changed");
}
Proposal propose(const ure::Root& system,const StorageTarget& target,const Value& request,const std::filesystem::path& original,const std::string& profile,
                 const Value& receipt,const Evidence& ev,bool preserve,bool fastboot) {
    need(request["keep_userdata"]==true,"Userdata must remain the first partition in its original pool");
    if(request.isMember("order"))need(request["order"].isArray()&&!request["order"].empty()&&request["order"][0]=="userdata","Advanced order must keep userdata first");
    const auto inspected=ure::multiboot_inspect(target,original,profile,&system);require_ownership(inspected,receipt,target.identity);
    need(!inspected["old_rows"].empty()&&inspected["old_rows"][0]["label"]=="userdata"&&ure::json(inspected["old_rows"][0]["start_lba"])==ure::json(inspected["original_userdata"]["start_lba"])&&inspected["old_rows"][0]["partuuid"]==inspected["original_userdata"]["partuuid"],"Existing userdata start/GUID/order must match original evidence");
    Proposal out;out.before=ure::gpt_regions(target.descriptor.get(),target.identity["logical_sector_bytes"].asUInt());Value layout;
    auto allocation=request;
    // Recovery's Android selector admits ext4/F2FS; Fedora's Btrfs/XFS use identical GPT geometry.
    if(allocation["userdata_filesystem"]=="btrfs"||allocation["userdata_filesystem"]=="xfs")allocation["userdata_filesystem"]="ext4";
    out.after=ure::multiboot_gpt_regions(target,allocation,original,profile,layout,&system);
    layout["rows"][0]["filesystem"]=request["userdata_filesystem"];
    validate_protected(target,inspected,out.before,out.after);
    need(layout["rows"].size()>0&&layout["rows"][0]["role"]=="userdata"&&ure::json(layout["rows"][0]["start_lba"])==ure::json(inspected["original_userdata"]["start_lba"])&&
         layout["rows"][0]["partuuid"]==inspected["original_userdata"]["partuuid"],"Userdata start/GUID/order cannot change");
    const auto begin=inspected["pool"]["offset"].asUInt64(),capacity=inspected["pool"]["original_bytes"].asUInt64();
    const auto new_bytes=layout["rows"][0]["bytes"].asUInt64();std::uint64_t cursor=begin;
    for(const auto& row:layout["rows"]) {const auto at=row["offset"].asUInt64(),size=row["bytes"].asUInt64();need(at==cursor&&at>=begin&&at-begin<=capacity&&size>0&&size<=capacity-(at-begin),"New partition escapes or precedes original userdata");cursor+=size;}
    Value doc;doc["schema"]="org.uke.control.layout.plan.v1";doc["engine_version"]=engine_version;doc["recovery_source"]=recovery_source;
    doc["target_identity"]=target.identity;doc["request"]=request;doc["original_backup"]="original-gpt";doc["firmware_profile"]=profile;
    doc["original_manifest_sha256"]=inspected["original_manifest_sha256"];doc["pool"]=inspected["pool"];doc["before"]=descriptions(out.before);doc["after"]=descriptions(out.after);
    doc["old_rows"]=inspected["old_rows"];doc["new_rows"]=layout["rows"];doc["protected_records"]=inspected["protected_records"];
    doc["preserve_userdata"]=preserve;doc["finish"]=fastboot?"fastboot":"stay";doc["oem_payload_writes"]=false;doc["blockers"]=Value(Json::arrayValue);
    auto block=[&](const char* message){doc["blockers"].append(message);};
    bool children=false,active_children=false,root_removed=false;
    for(const auto& row:inspected["old_rows"])if(row["label"]!="userdata") {
        children=true;const auto index=row["index"].asUInt();active_children=active_children||ev.mounted.contains(index)||ev.swaps.contains(index)||ev.holders.contains(index);root_removed=root_removed||index==ev.root_index;
    }
    doc["data_loss"]=!preserve||children;doc["confirmations_required"]=doc["data_loss"].asBool()?2:1;
    doc["userdata_filesystem"]=ev.filesystem;doc["userdata_uuid"]=ev.filesystem_uuid;doc["userdata_filesystem_bytes"]=Json::UInt64(ev.filesystem_bytes);
    const bool shrink=ev.filesystem_bytes>new_bytes,grow=ev.filesystem_bytes<new_bytes;
    if(preserve&&(ev.filesystem.empty()||ev.filesystem!=request["userdata_filesystem"].asString()||ev.filesystem_uuid.empty()))block("Preservation requires a verified unchanged userdata filesystem and UUID");
    if(preserve&&shrink&&(ev.filesystem=="xfs"))block("This filesystem has no admitted shrink adapter; erase/recreate needs two new confirmations");
    if(preserve&&!ev.single_device)block("Only verified single-device filesystems are admitted");
    if(preserve&&ev.external_journal)block("External filesystem journals/devices are outside the userdata-only write boundary");
    if(preserve&&ev.filesystem!="ext4"&&ev.filesystem!="f2fs"&&ev.filesystem!="btrfs"&&ev.filesystem!="xfs")block("No reviewed userdata preservation adapter for this filesystem");
    if(preserve&&!ev.filesystem_size_verified)block("Filesystem geometry is not independently verified");
    bool shutdown=ev.require_shutdown||root_removed||active_children||ev.holders.contains(ev.userdata_index)||ev.swaps.contains(ev.userdata_index)||(!preserve&&ev.mounted.contains(ev.userdata_index));
    if(preserve&&!ev.userdata_mount.empty()) {
        if(shrink&&ev.filesystem=="ext4"&&ev.filesystem_bytes>new_bytes)shutdown=true;
        if(grow&&ev.filesystem=="f2fs")shutdown=true;
        if(shrink&&ev.filesystem=="btrfs"&&ev.swaps.contains(ev.userdata_index))shutdown=true;
    }
    const bool same=ure::json(descriptions(out.before))==ure::json(descriptions(out.after));
    const bool filesystem_full=ev.filesystem_size_verified&&ev.filesystem_bytes<=new_bytes&&new_bytes-ev.filesystem_bytes<16*1048576;
    doc["no_changes_required"]=same&&preserve&&filesystem_full;
    for(const auto& row:layout["rows"])if(doc["no_changes_required"]!=true&&(row["role"]!="userdata"||!preserve)) {
        bool available=false;for(const auto& cap:native_capabilities())if(cap["filesystem"]==row["filesystem"])available=cap["available"].asBool();
        if(!available)block(("Native format/check adapter is unavailable: "+row["filesystem"].asString()).c_str());
        const auto type=row["filesystem"].asString();
        const std::uint64_t minimum=type=="btrfs"?256*1048576ULL:(type=="f2fs"||type=="xfs"||type=="fat32")?512*1048576ULL:32*1048576ULL;
        if(row["bytes"].asUInt64()<minimum)block("Volume is too small for its admitted native formatter");
    }
    if(doc["no_changes_required"]==true){doc["data_loss"]=false;doc["confirmations_required"]=0;}
    doc["root_partition_removed"]=root_removed||(!preserve&&ev.root_index==ev.userdata_index);
    if(doc["root_partition_removed"]==true&&!fastboot)block("Replacing the running root requires final fastboot");
    doc["route"]=doc["blockers"].empty()?(doc["no_changes_required"]==true?"none":shutdown?"shutdown":"direct"):"blocked";
    doc["userdata_mounted"]=!ev.userdata_mount.empty();doc["userdata_mount"]=ev.userdata_mount;
    doc["userdata_node"]=ev.userdata_node;doc["userdata_index"]=ev.userdata_index;
    doc["sha256"]=seal(doc);out.document=doc;return out;
}
std::string preview(const Value& p) {
    std::ostringstream out;out<<"Uke boot layout\n\nOriginal userdata pool: "<<bytes(p["pool"]["original_bytes"].asUInt64())
        <<"\nProtected OEM records: "<<p["protected_records"].size()<<" (unchanged)\nExecution: "<<p["route"].asString()<<"\n\n";
    for(const auto& [key,title]:std::array<std::pair<const char*,const char*>,2>{{{"old_rows","Current layout"},{"new_rows","Proposed layout (userdata first)"}}}) {
        out<<title<<"\n";for(const auto& row:p[key])out<<"  "<<std::left<<std::setw(17)<<row["label"].asString()<<std::setw(13)<<bytes(row["bytes"].asUInt64())<<row["filesystem"].asString()<<'\n';out<<'\n';
    }
    out<<"Preserve userdata files: "<<(p["preserve_userdata"].asBool()?"yes":"NO - ERASE")
       <<"\nOther existing project volumes: "<<(p["old_rows"].size()>1?"will be removed/recreated":"none")
       <<"\nFinish: "<<p["finish"].asString()<<"\nPlan: "<<p["sha256"].asString()<<"\n";
    for(const auto& reason:p["blockers"])out<<"BLOCKED: "<<reason.asString()<<'\n';
    return out.str();
}
}
