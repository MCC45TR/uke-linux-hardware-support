// SPDX-License-Identifier: MIT
// Recovery checkpoint format: orangefox_device_xiaomi_uke, dualboot_host_journal.hpp (Apache-2.0).
#include "layout.hpp"
#include <fcntl.h>
#include <linux/memfd.h>
#include <map>
#include <sys/mman.h>
#include <unistd.h>

namespace uke::layout {
namespace fs=std::filesystem;
namespace {
void write_metadata(int fd,const std::string& bytes,std::uint64_t at) {
    size_t done=0;while(done<bytes.size()){const auto n=pwrite(fd,bytes.data()+done,bytes.size()-done,at+done);if(n<0&&errno==EINTR)continue;need(n>0,"Cannot construct recovery metadata view");done+=size_t(n);}
}
std::uint32_t le32(const std::string& bytes,size_t at) {
    need(at<=bytes.size()&&4<=bytes.size()-at,"Truncated recovery GPT header");std::uint32_t out=0;for(unsigned i=0;i<4;++i)out|=std::uint32_t(static_cast<unsigned char>(bytes[at+i]))<<(8*i);return out;
}
std::uint32_t crc32(std::string_view bytes) {
    std::uint32_t out=UINT32_MAX;for(unsigned char c:bytes){out^=c;for(unsigned bit=0;bit<8;++bit)out=(out>>1)^((out&1)?0xedb88320U:0);}return out^UINT32_MAX;
}
void put32(std::string& bytes,size_t at,std::uint32_t value){need(at<=bytes.size()&&4<=bytes.size()-at,"Truncated recovery GPT header");for(unsigned i=0;i<4;++i)bytes[at+i]=char(value>>(8*i));}
}
Value import_recovery_checkpoint(const ure::Root& system,const StorageTarget& target,const Value& checkpoint,const fs::path& destination) {
    need(checkpoint["schema"]==1&&checkpoint["format"]=="ure-adb-dualboot-checkpoint"&&
         checkpoint["phase"]=="COMMITTED_REBOOT_REQUIRED"&&checkpoint["sequence"].isUInt64()&&checkpoint["sequence"].asUInt64()>0&&
         ure::identifier(checkpoint["session"].asString())&&ure::hash_valid(checkpoint["plan_sha256"].asString())&&
         checkpoint["files"].isArray()&&checkpoint["files"].size()==12,"A complete final recovery checkpoint is required");
    std::map<std::string,std::string> files;
    for(const auto& row:checkpoint["files"]) {
        const auto name=row["name"].asString(),hex=row["hex"].asString();
        need((name=="plan.json"||name=="state.json"||name.starts_with("before-")||name.starts_with("after-"))&&
             ure::identifier(name)&&!hex.empty()&&hex.size()%2==0&&hex.size()<=2*1024*1024,"Invalid recovery checkpoint member");
        auto digit=[](char c)->unsigned {need((c>='0'&&c<='9')||(c>='a'&&c<='f'),"Invalid recovery checkpoint encoding");return c<='9'?c-'0':c-'a'+10;};
        std::string bytes;bytes.reserve(hex.size()/2);
        for(size_t i=0;i<hex.size();i+=2)bytes+=char((digit(hex[i])<<4)|digit(hex[i+1]));
        need(row["sha256"]==ure::sha256(bytes)&&files.emplace(name,std::move(bytes)).second,"Duplicate or changed recovery checkpoint member");
    }
    need(files.contains("plan.json")&&files.contains("state.json"),"Recovery checkpoint lacks its plan or terminal state");
    const auto plan=ure::parse_json(files.at("plan.json")),state=ure::parse_json(files.at("state.json"));
    const auto hash=checkpoint["plan_sha256"].asString();const auto& previous=plan["target_identity"];
    need(plan["schema"]==1&&plan["operation"]=="dualboot.setup"&&plan["plan_sha256"]==hash&&seal(plan,"plan_sha256")==hash&&
         plan["gpt"]["plan_sha256"]==seal(plan["gpt"],"plan_sha256")&&
         ure::json(plan["gpt"]["target_identity"])==ure::json(previous),"Recovery plan binding differs");
    need(state["schema"]==1&&state["format"]=="ure-live-dualboot-journal"&&state["phase"]==checkpoint["phase"]&&state["plan_sha256"]==hash,
         "Recovery checkpoint is not a completed creation record");
    if(target.identity["kind"]=="regular-image") {
        for(const auto* key:{"path","file_device","file_inode","bytes","logical_sector_bytes","disk_guid","uid","gid","mode"})
            need(!previous[key].isNull()&&ure::json(previous[key])==ure::json(target.identity[key]),"Recovery fixture image identity differs");
    }
    else {
        need(previous["unit_identity_available"]==true&&target.identity["unit_identity_available"]==true,"Recovery import requires an independently identified unit");
        for(const auto* key:{"unit_identity_sha256","lun_address","disk_guid","bytes","logical_sector_bytes"})
            need(!previous[key].isNull()&&ure::json(previous[key])==ure::json(target.identity[key]),"Recovery checkpoint belongs to another unit, disk or LUN");
    }
    const auto size=target.identity["bytes"].asUInt64();need(size>0&&size<=INT64_MAX,"Invalid original disk capacity");
    ure::Fd original(memfd_create("uke-recovery-original",MFD_CLOEXEC)),historical(memfd_create("uke-recovery-completed",MFD_CLOEXEC));
    need(original.get()>=0&&historical.get()>=0&&!ftruncate(original.get(),size)&&!ftruncate(historical.get(),size),"Cannot construct original metadata views");
    std::map<std::string,std::string> current;
    const std::set<std::string> names{"protective_mbr","primary_header","primary_table","backup_header","backup_table"};
    for(const auto* side:{"before","after"}) {
        const auto& ranges=plan["gpt"][side];need(ranges.isArray()&&ranges.size()==5,"Recovery requires both complete GPT copies");std::set<std::string> seen;
        for(const auto& range:ranges) {
            const auto name=range["name"].asString(),member=std::string(side)+"-"+name+".bin";
            need(names.contains(name)&&seen.insert(name).second&&files.contains(member),"Missing or duplicate recovery GPT region");
            const auto& bytes=files.at(member);const auto at=range["offset"].asUInt64();
            need(range["bytes"].asUInt64()==bytes.size()&&range["sha256"]==ure::sha256(bytes)&&at<=size&&bytes.size()<=size-at,
                 "Recovery GPT metadata escapes its disk or differs from its plan");
            if(std::string_view(side)=="after"){current[name]=ure::storage_read(target.descriptor.get(),at,bytes.size());write_metadata(historical.get(),bytes,at);}
            else write_metadata(original.get(),bytes,at);
        }
    }
    const auto sector=target.identity["logical_sector_bytes"].asUInt();
    const auto original_table=ure::gpt_inspect(original.get(),sector),completed_table=ure::gpt_inspect(historical.get(),sector);
    need(original_table["healthy"]==true&&completed_table["healthy"]==true&&ure::gpt_inspect(target.descriptor.get(),sector)["healthy"]==true,"Recovery or current GPT is unhealthy");
    need(current.at("protective_mbr")==files.at("after-protective_mbr.bin"),"Current protective MBR differs from recovery evidence");
    Value transitions(Json::arrayValue);auto rebased=files;
    for(const auto* table_name:{"primary_table","backup_table"}) {
        const auto after_name=std::string("after-")+table_name+".bin",before_name=std::string("before-")+table_name+".bin";
        auto expected=files.at(after_name);auto& before=rebased.at(before_name);const auto& actual=current.at(table_name);
        for(const auto* stem:{"countrycode","vbmeta_system"}) {
            unsigned a=0,b=0;for(const auto& row:completed_table["partitions"]){if(row["label"]==std::string(stem)+"_a")a=row["index"].asUInt();if(row["label"]==std::string(stem)+"_b")b=row["index"].asUInt();}
            if(!a&&!b)continue;
            need(a>0&&b>0&&a!=b,"Incomplete OEM slot pair in recovery evidence");
            const auto x=size_t(a-1)*128,y=size_t(b-1)*128;need(x+128<=expected.size()&&y+128<=expected.size()&&x+128<=before.size()&&y+128<=before.size(),"OEM slot pair escapes recovery metadata");
            const auto old_a=expected.substr(x,128),old_b=expected.substr(y,128);
            if(actual.substr(x,128)==old_a&&actual.substr(y,128)==old_b)continue;
            need(before.substr(x,128)==old_a&&before.substr(y,128)==old_b&&((old_a[54]^old_b[54])&4),"OEM slot provenance is not a reciprocal active pair");
            auto new_a=old_a,new_b=old_b;new_a.replace(0,16,old_b.substr(0,16));new_b.replace(0,16,old_a.substr(0,16));new_a[54]^=4;new_b[54]^=4;
            need(actual.substr(x,128)==new_a&&actual.substr(y,128)==new_b,"OEM slot changes exceed a reciprocal type-GUID and active-bit transition");
            expected.replace(x,128,new_a);expected.replace(y,128,new_b);before.replace(x,128,new_a);before.replace(y,128,new_b);
            if(std::string_view(table_name)=="primary_table")transitions.append(stem);
        }
        need(expected==actual,"Current GPT contains changes outside verified OEM slot transitions");
    }
    for(const auto* header_name:{"primary_header","backup_header"}) {
        const auto& old=files.at(std::string("after-")+header_name+".bin");const auto& actual=current.at(header_name);need(old.size()==actual.size(),"Recovery GPT header size changed");
        for(size_t i=0;i<old.size();++i)if(!((i>=16&&i<20)||(i>=88&&i<92)))need(old[i]==actual[i],"Current GPT geometry differs from recovery evidence");
    }
    need(!fs::exists(destination),"Recovery import preserves an existing destination");ure::private_directory(destination,true);
    const auto firmware=plan["firmware_profile"].asString();need(ure::identifier(firmware),"Invalid recovery firmware profile");
    StorageTarget original_target{std::move(original),target.identity};
    if(!transitions.empty()) {
        backup(original_target,destination/"recovery-original-gpt",firmware);
        for(const auto& [header_name,table_name]:{std::pair{"primary_header","primary_table"},std::pair{"backup_header","backup_table"}}) {
            auto& header=rebased.at(std::string("before-")+header_name+".bin");const auto& table=rebased.at(std::string("before-")+table_name+".bin");
            const auto count=le32(header,80),entry=le32(header,84),length=le32(header,12);need(entry==128&&count>0&&count<=table.size()/entry&&length>=92&&length<=header.size(),"Invalid original GPT bounds");
            put32(header,88,crc32(std::string_view(table).substr(0,size_t(count)*entry)));put32(header,16,0);put32(header,16,crc32(std::string_view(header).substr(0,length)));
        }
        for(const auto& range:plan["gpt"]["before"])write_metadata(original_target.descriptor.get(),rebased.at("before-"+range["name"].asString()+".bin"),range["offset"].asUInt64());
    }
    backup(original_target,destination/"original-gpt",firmware);
    const auto inspection=ure::multiboot_inspect(target,destination/"original-gpt",firmware,&system);
    need(inspection["existing_multiboot"]==true,"Recovery checkpoint does not establish current project children");
    Value receipt;receipt["schema"]="org.uke.control.layout.ownership.v1";receipt["completed"]=true;
    receipt["disk_guid"]=inspection["current_table"]["disk_guid"];receipt["original_manifest_sha256"]=inspection["original_manifest_sha256"];
    receipt["unit_identity_sha256"]=target.identity["unit_identity_sha256"];receipt["lun_address"]=target.identity["lun_address"];
    receipt["owned_rows"]=Value(Json::arrayValue);for(const auto& row:inspection["old_rows"])if(row["label"]!="userdata")receipt["owned_rows"].append(row);
    receipt["provenance"]="verified completed recovery checkpoint";receipt["recovery_plan_sha256"]=hash;receipt["recovery_checkpoint_sha256"]=ure::sha256(ure::json(checkpoint));
    receipt["preserved_oem_slot_transitions"]=transitions;
    receipt["sha256"]=seal(receipt);require_ownership(inspection,receipt,target.identity);
    save_private(destination/"recovery-checkpoint.json",checkpoint);save_private(destination/"ownership.json",receipt);return receipt;
}
}
