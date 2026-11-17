#!/usr/bin/bash
# COPR only: actual production startup code, real private receipt files, injected block metadata.
set -Eeuo pipefail
[[ $# == 1 && $(uname -m) == aarch64 ]]
root=$(realpath "$1")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
ulimit -c 0
cat > "$work/test.cpp" <<'CPP'
#include "swap-ready.hpp"
#include <cassert>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
namespace realfs=std::filesystem;
static realfs::path fixture;
static std::string boot_id="11111111-1111-1111-1111-111111111111\n", parts="major minor  #blocks  name\n253 0 8806400 dm-0\n";
static std::string selected="half", uuid="UKE-SWAP-v1-259-15\n", events="0", type="swap\n", label="linux_swap\n";
static bool mapper=true,block=true,foreign=false,helper_trusted=true,changed_fd=false,apply_failure=false,change_inventory=false;
static uint64_t capacity=9017753600ULL,active_size=9017749504ULL;
static unsigned full_calls=0,command_calls=0;
static bool managed=false;
static realfs::path mapped(const realfs::path& path){return fixture/path.relative_path();}
namespace fs {
using path=realfs::path;
static bool exists(const path& path){assert(path=="/dev/mapper/linux_swap");return mapper;}
static path canonical(const path& path){assert(path=="/dev/mapper/linux_swap");return "/dev/dm-0";}
}
namespace uke {
static const fs::path state="/var/lib/uke-control";
static void need(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
static void board(){}struct Lock{explicit Lock(bool=false){}};
struct Fd{int n;~Fd(){if(n>=0)close(n);}};
struct Result{int status;std::string output;};
struct Swap{std::string path;uint64_t size,used;};
static void directory(const fs::path& path){realfs::create_directories(mapped(path));assert(!chmod(mapped(path).c_str(),0700));}
static void atomic_file(const fs::path& path,const std::string& value){directory(path.parent_path());std::ofstream(mapped(path))<<value;assert(!chmod(mapped(path).c_str(),0600));}
static std::string read(const std::string& path,size_t limit=1024*1024){
 if(path=="/proc/sys/kernel/random/boot_id")return boot_id;
 if(path=="/proc/partitions")return parts;
 if(path=="/sys/dev/block/253:0/dm/uuid")return uuid;
 std::ifstream input(mapped(path));if(!input)return "unavailable\n";
 std::string value{std::istreambuf_iterator<char>(input),{}};assert(value.size()<limit);return value;
}
static int stat_fixture(const char* path,struct stat* st){
 assert(std::string(path)=="/dev/dm-0");st->st_mode=block?S_IFBLK:S_IFREG;st->st_rdev=makedev(253,0);return 0;
}
static int lstat_fixture(const char* path,struct stat* st){
 if(std::string(path)=="/usr/libexec/uke-control/super-swap/prepare.sh"){
  st->st_mode=S_IFREG|(helper_trusted?0755:0777);st->st_uid=0;return 0;
 }
 const int result=::lstat(mapped(path).c_str(),st);if(!result)st->st_uid=foreign?1001:0;return result;
}
static int open_fixture(const char* path,int flags){assert(std::string(path)=="/dev/dm-0"&&(flags&O_NOFOLLOW));return ::open("/dev/null",O_RDONLY|O_CLOEXEC);}
static int fstat_fixture(int fd,struct stat* st){assert(fd>=0);st->st_mode=S_IFBLK;st->st_rdev=makedev(253,changed_fd?1:0);return 0;}
static int ioctl_fixture(int,unsigned long request,uint64_t* value){assert(request==BLKGETSIZE64);*value=capacity;return 0;}
static int unlink_fixture(const char* path){return ::unlink(mapped(path).c_str());}
static std::string policy(){return selected;}
static uint64_t memory(const std::string& key){assert(key=="MemTotal:");return 8ULL<<30;}
static uint64_t swap_mib(const std::string&,uint64_t){return 4096;}
static void apply_locked(const std::string& choice){assert(choice==selected);++full_calls;if(change_inventory)parts+="253 1 1024 dm-1\n";need(!apply_failure,"injected full admission refusal");}
static bool own_file(const std::string& path){return path=="/var/lib/uke-control/swap/swap-4096";}
static std::vector<Swap> active(){std::vector<Swap> out{{"/dev/dm-0",active_size,0}};if(managed)out.push_back({"/var/lib/uke-control/swap/swap-4096",4096,0});return out;}
static Result execute(const std::vector<std::string>& args,unsigned,size_t){
 ++command_calls;
 if(args[0]=="/usr/sbin/dmsetup")return {0,uuid.substr(0,uuid.size()-1)+":253:0:"+events+"\n"};
 assert(args[0]=="/usr/bin/blkid"&&args[1]=="-p"&&args.back()=="/dev/dm-0");
 assert(args[3]=="TYPE"||args[3]=="LABEL");return {0,args[3]=="TYPE"?type:label};
}
#define stat(path,st) stat_fixture(path,st)
#define lstat(path,st) lstat_fixture(path,st)
#define open(path,flags) open_fixture(path,flags)
#define fstat(fd,st) fstat_fixture(fd,st)
#define ioctl(fd,request,value) ioctl_fixture(fd,request,value)
#define unlink(path) unlink_fixture(path)
CPP
awk '/^static const fs::path swap_receipt=/{copy=1} /^std::string inventory\(/{copy=0} copy' "$root/src/backend.cpp" >> "$work/test.cpp"
cat >> "$work/test.cpp" <<'CPP'
#undef stat
#undef lstat
#undef open
#undef fstat
#undef ioctl
#undef unlink
}
int main(int argc,char** argv){
 assert(argc==2);fixture=argv[1];using namespace uke;directory(state);
 swap_ready();assert(full_calls==1);swap_ready();assert(full_calls==1&&command_calls==3);
 // A manual apply always performs full admission, even with a valid startup receipt.
 swap_apply();assert(full_calls==2);swap_ready();assert(full_calls==2);
 auto fallback=[&]{const auto before=full_calls;swap_ready();assert(full_calls==before+1);};
 auto reuse=[&]{const auto before=full_calls;swap_ready();assert(full_calls==before);};
 boot_id[0]='2';fallback();reuse();
 parts+="253 1 1024 dm-1\n";fallback();reuse();
 selected="full";fallback();reuse();
 events="1";fallback();events="0";reuse();
 type="ext4\n";fallback();type="swap\n";reuse();
 label="foreign\n";fallback();label="linux_swap\n";reuse();
 active_size-=4096;fallback();active_size+=4096;reuse();
 capacity+=4096;fallback();capacity-=4096;reuse();
 changed_fd=true;fallback();changed_fd=false;reuse();
 managed=true;fallback();managed=false;reuse();
 helper_trusted=false;fallback();helper_trusted=true;reuse();
 foreign=true;fallback();foreign=false;reuse();
 block=false;fallback();block=true;reuse();
 mapper=false;fallback();mapper=true;reuse();
 const auto original_uuid=uuid;uuid="foreign\n";fallback();uuid=original_uuid;reuse();
 assert(!chmod(mapped(swap_receipt).c_str(),0644));fallback();reuse();
 assert(!chmod(mapped(swap_receipt.parent_path()).c_str(),0755));fallback();reuse();
 const auto old=mapped(state/"receipt-old");realfs::create_hard_link(mapped(swap_receipt),old);fallback();reuse();
 assert(realfs::exists(old));realfs::remove(mapped(swap_receipt));realfs::create_symlink(old,mapped(swap_receipt));fallback();reuse();assert(realfs::is_regular_file(old));
 // A block inventory change while full verification runs cannot leave a reusable receipt.
 change_inventory=true;events="1";fallback();assert(!realfs::exists(mapped(swap_receipt)));change_inventory=false;events="0";fallback();reuse();
 // Full refusal propagates and invalidates prior success. No file fallback is added here.
 apply_failure=true;events="1";bool rejected=false;try{swap_ready();}catch(const std::exception&){rejected=true;}assert(rejected&&!realfs::exists(mapped(swap_receipt)));apply_failure=false;events="0";fallback();reuse();
 swap_set("full");assert(!realfs::exists(mapped(swap_receipt)));fallback();reuse();
 std::cout<<"PASS: actual swap startup admission, current-boot/inventory/policy binding, unchanged active mapper, ownership/link/mode refusals, manual full admission and failure propagation\n";
}
CPP
for compiler in g++ clang++;do
 for mode in plain sanitized;do
  flags=(-std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror)
  [[ $mode != sanitized ]] || flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  "$compiler" "${flags[@]}" -I"$root/src" "$work/test.cpp" -o "$work/check"
  readelf -h "$work/check" | grep -F AArch64
  mkdir "$work/$compiler-$mode"
  timeout --kill-after=2s 20s "$work/check" "$work/$compiler-$mode"
 done
done
