// SPDX-License-Identifier: MIT
#include "storage.hpp"
#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
namespace fs=std::filesystem;
void put(const fs::path& p,const std::string& s){fs::create_directories(p.parent_path());std::ofstream(p)<<s;}
void refuses(const std::function<void()>& f){bool bad=false;try{f();}catch(...){bad=true;}assert(bad);}
int main(){
    char tmp[]="/tmp/uke-storage-XXXXXX";auto created=mkdtemp(tmp);assert(created);fs::path root=created;
    try {
        auto disk=root/"sys/devices/block/sda",part=disk/"sda1";
        for(const auto& p:{disk,part}){put(p/"dev",p==disk?"8:0":"8:1");put(p/"size","1024");put(p/"ro","0");fs::create_directories(p/"holders");}
        put(part/"partition","1");put(part/"start","128");put(part/"size","512");put(disk/"queue/logical_block_size","512");fs::create_directories(root/"sys/class/block");
        fs::create_symlink(disk,root/"sys/class/block/sda");fs::create_symlink(part,root/"sys/class/block/sda1");
        put(root/"proc/self/mountinfo","1 0 8:1 / / rw - ext4 /dev/sda1 rw\n");
        put(root/"proc/swaps","Filename Type Size Used Priority\n");
        auto all=uke::storage::snapshot(root,"/dev/sda");
        assert(all.find("/dev/sda1")!=std::string::npos && all.find("\"filesystem\":\"ext4\"")!=std::string::npos);
        assert(all.find("\"chroot_ready\":true")==std::string::npos);
        assert(all.find("device_identity_unverified")!=std::string::npos);
        fs::create_directories(root/"dev");
        // Fake nodes are only lstat'ed, never opened or mounted.
        if(::mknod((root/"dev/sda1").c_str(),S_IFBLK|0600,makedev(8,1))==0) {
            auto reason=[&](const char* value){assert(uke::storage::snapshot(root,"/dev/sda1").find(value)!=std::string::npos);};
            reason("mounted_device");
            put(root/"proc/self/mountinfo","1 0 0:1 / / rw - tmpfs tmpfs rw\n");
            reason("filesystem_inspection_required");
            put(part/"ro","1");reason("read_only_or_unknown");put(part/"ro","0");
            put(part/"holders/dm-0/dev","253:0");reason("device_has_holders");fs::remove_all(part/"holders/dm-0");
            put(root/"proc/swaps","Filename Type Size Used Priority\n/dev/sdb1 partition 1 0 -2\n");reason("swap_dependencies_unknown");
            put(root/"proc/swaps","Filename Type Size Used Priority\n");
            put(root/"proc/self/mountinfo","malformed");reason("mount_inventory_unknown");
            fs::remove(root/"dev/sda1");::mknod((root/"dev/sda1").c_str(),S_IFBLK|0600,makedev(8,2));reason("device_identity_unverified");
            fs::remove(root/"dev/sda1");
        } else std::cout<<"Fake block-node admission cases skipped: mknod unavailable\n";
        auto one=uke::storage::snapshot(root,"/dev/sda1");assert(one.find("\"path\":\"/dev/sda\"")==std::string::npos);
        for(const auto& invalid:{"/dev/sda;id","/dev/../sda","/dev/disk/by-id/test","/dev/missing","sda"})refuses([&]{uke::storage::snapshot(root,invalid);});
        auto shrinking=uke::storage::plan(root,"/dev/sda1","resize","131072");
        assert(shrinking.find("offline_filesystem_shrink_support_and_minimum_required")!=std::string::npos);
        auto sibling=disk/"sda2";put(sibling/"partition","2");put(sibling/"start","700");put(sibling/"size","100");put(sibling/"dev","8:2");
        assert(uke::storage::plan(root,"/dev/sda1","resize","409600").find("requested_extent_overlaps_partition")!=std::string::npos);
        assert(uke::storage::plan(root,"/dev/sda1","format","ext4").find("all_target_data_would_be_destroyed")!=std::string::npos);
        refuses([&]{uke::storage::plan(root,"/dev/sda","delete");});
        refuses([&]{uke::storage::plan(root,"/dev/sda1","resize","513");});
        refuses([&]{uke::storage::plan(root,"/dev/sda1","resize","18446744073709551616");});
        refuses([&]{uke::storage::plan(root,"/dev/sda1","execute");});
        put(part/"dev","8:1 garbage");refuses([&]{uke::storage::snapshot(root);});put(part/"dev","8:1");
        put(root/"proc/self/mountinfo","malformed");assert(uke::storage::snapshot(root).find("\"chroot_ready\":true")==std::string::npos);
        fs::remove(root/"sys/class/block/sda1");refuses([&]{uke::storage::snapshot(root,"/dev/sda1");});
        fs::remove_all(root);
    }catch(...){fs::remove_all(root);throw;}
    std::cout<<"Storage inventory: disk children, selection validation, unknown identity, mounted filesystem, malformed and stale inputs passed\n";
}
