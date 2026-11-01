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
    need(previous["unit_identity_available"]==true&&target.identity["unit_identity_available"]==true,"Recovery import requires an independently identified unit");
    for(const auto* key:{"unit_identity_sha256","lun_address","disk_guid","bytes","logical_sector_bytes"})
        need(!previous[key].isNull()&&ure::json(previous[key])==ure::json(target.identity[key]),"Recovery checkpoint belongs to another unit, disk or LUN");
    const auto size=target.identity["bytes"].asUInt64();need(size>0&&size<=INT64_MAX,"Invalid original disk capacity");
    ure::Fd original(memfd_create("uke-recovery-original",MFD_CLOEXEC));need(original.get()>=0&&!ftruncate(original.get(),size),"Cannot construct original metadata view");
    const std::set<std::string> names{"protective_mbr","primary_header","primary_table","backup_header","backup_table"};
    for(const auto* side:{"before","after"}) {
        const auto& ranges=plan["gpt"][side];need(ranges.isArray()&&ranges.size()==5,"Recovery requires both complete GPT copies");std::set<std::string> seen;
        for(const auto& range:ranges) {
            const auto name=range["name"].asString(),member=std::string(side)+"-"+name+".bin";
            need(names.contains(name)&&seen.insert(name).second&&files.contains(member),"Missing or duplicate recovery GPT region");
            const auto& bytes=files.at(member);const auto at=range["offset"].asUInt64();
            need(range["bytes"].asUInt64()==bytes.size()&&range["sha256"]==ure::sha256(bytes)&&at<=size&&bytes.size()<=size-at,
                 "Recovery GPT metadata escapes its disk or differs from its plan");
            if(std::string_view(side)=="after")need(ure::storage_read(target.descriptor.get(),at,bytes.size())==bytes,"Current GPT differs from the completed recovery layout");
            else {size_t done=0;while(done<bytes.size()){const auto n=pwrite(original.get(),bytes.data()+done,bytes.size()-done,at+done);if(n<0&&errno==EINTR)continue;need(n>0,"Cannot construct original GPT proof");done+=size_t(n);}}
        }
    }
    need(ure::gpt_inspect(original.get(),target.identity["logical_sector_bytes"].asUInt())["healthy"]==true,"Original recovery GPT is unhealthy");
    need(!fs::exists(destination),"Recovery import preserves an existing destination");ure::private_directory(destination,true);
    const auto firmware=plan["firmware_profile"].asString();need(ure::identifier(firmware),"Invalid recovery firmware profile");
    StorageTarget original_target{std::move(original),target.identity};backup(original_target,destination/"original-gpt",firmware);
    const auto inspection=ure::multiboot_inspect(target,destination/"original-gpt",firmware,&system);
    need(inspection["existing_multiboot"]==true,"Recovery checkpoint does not establish current project children");
    Value receipt;receipt["schema"]="org.uke.control.layout.ownership.v1";receipt["completed"]=true;
    receipt["disk_guid"]=inspection["current_table"]["disk_guid"];receipt["original_manifest_sha256"]=inspection["original_manifest_sha256"];
    receipt["unit_identity_sha256"]=target.identity["unit_identity_sha256"];receipt["lun_address"]=target.identity["lun_address"];
    receipt["owned_rows"]=Value(Json::arrayValue);for(const auto& row:inspection["old_rows"])if(row["label"]!="userdata")receipt["owned_rows"].append(row);
    receipt["provenance"]="verified completed recovery checkpoint";receipt["recovery_plan_sha256"]=hash;receipt["recovery_checkpoint_sha256"]=ure::sha256(ure::json(checkpoint));
    receipt["sha256"]=seal(receipt);require_ownership(inspection,receipt,target.identity);
    save_private(destination/"recovery-checkpoint.json",checkpoint);save_private(destination/"ownership.json",receipt);return receipt;
}
}
