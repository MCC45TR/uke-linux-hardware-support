// SPDX-License-Identifier: MIT
#pragma once
#include "layout-fixture.hpp"
namespace uke::layout {
inline void vm_power(){need(read("/sys/class/dmi/id/sys_vendor").find("QEMU")!=std::string::npos,"Disposable QEMU guest required");}
inline Inspection vm_inspect() {
    vm_power();ure::Root system("/");auto graph=ure::storage_graph(system);Value userdata;
    for(const auto& row:graph["objects"])if(row["parent_lun_name"]=="sda"&&row["label"]=="userdata")userdata=row;
    need(userdata.isObject(),"Disposable userdata is unavailable");
    auto target=ure::storage_select(system,"sysfs:"+userdata["parent_lun_sysfs"].asString());
    need(target.identity["bytes"]==Json::UInt64(fixture::capacity)&&target.identity["logical_sector_bytes"]==4096&&target.identity["unit_identity_available"]==true,"Unexpected VM disk identity");
    return inspect_storage(std::move(target),graph,userdata);
}
}
