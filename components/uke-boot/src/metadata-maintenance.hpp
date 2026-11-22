// SPDX-License-Identifier: MIT
#pragma once
#include "slot-control.hpp"

// Call only after running-image/UFS/selected-slot and early-firmware admission.
// The caller owns the updater and disk locks plus the shutdown inhibitor.
static bool repair_unaccepted_slot_crc(
        const std::vector<std::pair<std::string,slot::Disk*>> &disks,
        const fs::path &state,bool preview) {
    std::vector<std::pair<std::string,slot::Disk*>> changed;
    std::set<std::string> names;
    std::set<slot::Disk*> descriptors;
    for(const auto &[name,disk]:disks) {
        need(token(name) && names.emplace(name).second && disk && descriptors.emplace(disk).second,
             "Ambiguous metadata maintenance target");
        // Maintenance cannot commit a pending success, fallback or slot switch.
        need(disk->expected.size()==disk->parts.size(),"Metadata maintenance partition set changed");
        for(const auto &[part,value]:disk->parts)
            need(disk->expected.at(part)==std::make_pair(value.type,value.attrs),
                 "Metadata maintenance cannot change slot flags or partition types");
        if(disk->needs_backup_crc_repair())changed.emplace_back(name,disk);
    }
    if(changed.empty())return false;
    if(preview)return true;
    for(const auto &[name,disk]:changed)
        disk->backup(state/("crc-only-"+digest(disk->before.entries)+"-"+name+".gpt"));
    try {
        for(const auto &[name,disk]:changed){(void)name;disk->commit();}
    } catch(...) {
        for(const auto &[name,disk]:changed){(void)name;disk->restore();}
        throw;
    }
    return true;
}
