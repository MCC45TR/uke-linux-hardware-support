// SPDX-License-Identifier: MIT
// Actual Mounts RAII, simulated mount syscalls, disposable temporary directories.
#include <sys/mount.h>
#include <sched.h>
#include <cstdlib>
#include <filesystem>
#include <cassert>
#include <vector>
#include <string>
namespace fs=std::filesystem;
static std::vector<std::string> events;
static bool fail_add=false,fail_cleanup=false;
static int fake_setns(int fd,int type){assert(fd>=0&&type==CLONE_NEWNS);return 0;}
static int fake_unshare(int flags){assert(flags==CLONE_NEWNS);events.push_back("namespace");return 0;}
static int fake_mount(const char*,const char* target,const char* type,unsigned long flags,const void*) {
    if(std::string(target)=="/"){assert(flags==(MS_REC|MS_PRIVATE));events.push_back("private");return 0;}
    if(type&&std::string(type)=="tmpfs"){events.push_back("stage");return 0;}
    events.push_back("target");if(fail_add){errno=EIO;return -1;}return 0;
}
static int fake_umount(const char* target,int flags){assert(flags==0);events.push_back("unmount");if(fail_cleanup){errno=EBUSY;return -1;}const fs::path path=target;if(path.filename().string().starts_with("uke-mount-test-"))fs::remove_all(path/"uke-chroot");return 0;}
#define setns fake_setns
#define unshare fake_unshare
#define mount fake_mount
#define umount2 fake_umount
#include "../src/chroot.cpp"
#undef setns
#undef unshare
#undef mount
#undef umount2
int main(){
    using namespace uke::chroot_manager;
    char pattern[]="/tmp/uke-mount-test-XXXXXX";auto runtime=::mkdtemp(pattern);assert(runtime);
    {Mounts mounts(runtime);assert((events==std::vector<std::string>{"namespace","private","stage"}));mounts.add("test",mounts.root,"ext4",MS_RDONLY|MS_NOEXEC|MS_NOSUID|MS_NODEV,"noload");assert(mounts.cleanup());}
    events.clear();fail_add=true;bool failed=false;try{Mounts mounts(runtime);mounts.add("test",mounts.root,"ext4",MS_RDONLY,"noload");}catch(...){failed=true;}assert(failed&&events.back()=="unmount");fail_add=false;
    {Mounts mounts(runtime);auto path=mounts.stage;fail_cleanup=true;assert(!mounts.cleanup(false));fail_cleanup=false;fs::remove_all(path);}
    fs::remove_all(runtime);
    std::cout<<"Actual mount lifecycle: private propagation before staging, reverse cleanup, setup failure and busy cleanup reporting passed with syscall fixtures\n";
}
