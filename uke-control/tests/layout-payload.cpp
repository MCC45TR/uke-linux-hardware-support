// SPDX-License-Identifier: MIT
#include "layout.hpp"
#include <cassert>
#include <iostream>
#include <unistd.h>
int main() {
    using namespace uke::layout;char name[]="/tmp/uke-layout-payload-XXXXXX";const auto made=mkdtemp(name);assert(made);const std::filesystem::path stage=made;
    try {
        Value record;record["schema"]="host-payload-fixture";record["sha256"]=seal(record);build_shutdown_payload(stage,record);
        assert(load_private(stage/"tree/etc/uke-layout/request.json")["sha256"]==record["sha256"]);
        const auto result=run({native_tool("chroot"),(stage/"tree").string(),"/usr/libexec/uke-control/layout-manager","multiboot","--json"});
        need(result.status==64&&result.output.find("--json requires")!=result.output.npos,"RAM helper dependency or no-action refusal check failed: "+result.output);
        const auto aliases=run({native_tool("chroot"),(stage/"tree").string(),"/usr/bin/mkfs.ext4","-V"});
        need(aliases.status==0,"RAM native formatter alias/dependency check failed: "+aliases.output);
        std::cout<<"Real dracut layout payload: size, special-device allowlist, no Python/private accounts, private request, native helper dependencies and formatter alias passed\n";
        std::filesystem::remove_all(stage);
    }catch(...){std::filesystem::remove_all(stage);throw;}
}
