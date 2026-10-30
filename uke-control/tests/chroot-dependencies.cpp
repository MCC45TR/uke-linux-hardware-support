// SPDX-License-Identifier: MIT
// Real admission parser and geometry checks against disposable metadata fixtures.
#include "../src/chroot.cpp"
#include <cassert>
#include <functional>
using namespace uke::chroot_manager;
void put(const fs::path& path,const std::string& value){fs::create_directories(path.parent_path());std::ofstream(path)<<value;}
void refuses(const std::function<void()>& f){bool refused=false;try{f();}catch(...){refused=true;}assert(refused);}
int main(){
    char temp[]="/tmp/uke-chroot-deps-XXXXXX";const auto created=mkdtemp(temp);assert(created);fs::path root=created,disk=root/"sda",part=disk/"sda1",sibling=disk/"sda2";
    put(disk/"dev","8:0");put(disk/"size","4096");put(part/"partition","1");put(part/"start","128");put(part/"size","1024");
    fs::create_directories(disk/"holders");fs::create_directories(part/"holders");
    put(sibling/"partition","2");put(sibling/"start","2048");put(sibling/"size","1024");
    const auto mountfile=root/"mountinfo",swapfile=root/"swaps";
    put(mountfile,"1 0 8:2 / / rw - ext4 /dev/sda2 rw\n");put(swapfile,"Filename Type Size Used Priority\n");
    Fd mounts(::open(mountfile.c_str(),O_RDONLY|O_CLOEXEC));assert(mounts.n>=0);
    auto check=[&]{no_live_dependencies(part,"8:1",mounts.n,swapfile);};check(); // unrelated sibling may be mounted
    put(mountfile,"1 0 8:1 / /boot rw - ext4 /dev/sda1 rw\n");refuses(check);
    put(mountfile,"1 0 8:0 / / rw - ext4 /dev/sda rw\n");refuses(check);
    put(mountfile,"1 0 8:2\n");refuses(check);
    put(mountfile,"1 0 8:2 / / rw - ext4 /dev/sda2 rw\n");
    put(part/"holders/dm-0","held");refuses(check);fs::remove(part/"holders/dm-0");
    put(sibling/"start","512");refuses(check);put(sibling/"start","2048");
    const auto swap=root/"swap file";put(swap,"fixture");std::string escaped=swap.string();escaped.replace(escaped.find(' '),1,"\\040");
    put(swapfile,"Filename Type Size Used Priority\n"+escaped+" file 16 0 -2\n");check();
    struct stat st{};assert(stat(swap.c_str(),&st)==0);refuses([&]{no_live_dependencies(part,dev_string(st.st_dev),mounts.n,swapfile);});
    put(swapfile,"Filename Type Size Used Priority\n/dev/missing partition 16 0 -2\n");refuses(check);
    refuses([&]{unescape_mount_path("/tmp/\\000bad");});refuses([&]{unescape_mount_path("/tmp/\\0xx");});
    fs::remove_all(root);
    std::cout<<"Chroot dependency admission: live root/boot/whole-disk refusal, unrelated sibling admission, original-table parsing, holders, overlapping extents, escaped swapfiles and unknown swap refusal passed\n";
}
