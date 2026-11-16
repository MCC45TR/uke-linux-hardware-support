#!/usr/bin/bash
# COPR-native actual functions, with command/sysfs/stat fault injection only.
set -Eeuo pipefail
[[ $# == 1 && $(uname -m) == aarch64 ]]
root=$(realpath "$1")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
ulimit -c 0
cat > "$work/test.cpp" <<'CPP'
#include <cassert>
#include <deque>
#include <filesystem>
#include <set>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <vector>
static bool physical=false,block=true,trusted=true;
namespace fs {
using path=std::filesystem::path;
static bool exists(const path& p) {
 assert(p==path("/sys/class/block/null/partition"));return physical;
}
}
struct Result {int status;std::string output;};
static void need(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
static std::deque<std::pair<std::vector<std::string>,Result>> calls;
static Result execute(const std::vector<std::string>& args) {
 assert(!calls.empty()&&calls.front().first==args);
 auto result=calls.front().second;calls.pop_front();return result;
}
static int fixture_stat(const char* path,struct stat* st) {
 assert(std::string(path)=="/dev/null");st->st_mode=block?S_IFBLK:S_IFREG;return 0;
}
static int fixture_lstat(const char* path,struct stat* st) {
 assert(std::string(path)=="/usr/libexec/uke-control/super-swap/prepare.sh");
 st->st_mode=S_IFREG|(trusted?0755:0777);st->st_uid=0;return 0;
}
static std::set<std::string> candidates{"/dev/null"};
static std::set<std::string> partition_candidates(){return candidates;}
#define stat(path,st) fixture_stat(path,st)
#define lstat(path,st) fixture_lstat(path,st)
CPP
awk '/^static void super_swap_helper\(/{p=1} /^static std::set<std::string> partition_candidates\(/{p=0} p' "$root/src/backend.cpp" >> "$work/test.cpp"
awk '/^static std::string partition\(/{p=1} /^static bool own_file\(/{p=0} p' "$root/src/backend.cpp" >> "$work/test.cpp"
cat >> "$work/test.cpp" <<'CPP'
static const std::string helper="/usr/libexec/uke-control/super-swap/prepare.sh";
static void signature(int code=0,std::string output="swap\n") {
 calls.push_back({{"/usr/bin/blkid","-p","-s","TYPE","-o","value","/dev/null"},{code,output}});
}
static void refusal(auto operation) {
 bool refused=false;try{operation();}catch(const std::exception&){refused=true;}assert(refused);
}
int main() {
 physical=true;signature();assert(partition()=="/dev/null"&&calls.empty());
 physical=false;calls.push_back({{helper,"--verify","/dev/null"},{0,"verified"}});
 signature();assert(partition()=="/dev/null"&&calls.empty());
 calls.push_back({{helper,"--verify","/dev/null"},{1,"foreign mapper"}});
 refusal([]{partition();});assert(calls.empty());
 trusted=false;refusal([]{partition();});assert(calls.empty());trusted=true;
 block=false;refusal([]{partition();});assert(calls.empty());block=true;
 calls.push_back({{helper,"--verify","/dev/null"},{0,""}});signature(0,"ext4\n");
 refusal([]{partition();});assert(calls.empty());
 // Preparation refusal must precede discovery and the managed-file fallback.
 candidates.clear();calls.push_back({{helper,"--prepare"},{1,"invalid topology"}});
 refusal([]{prepared_partition();});assert(calls.empty());
 calls.push_back({{helper,"--prepare"},{0,"legacy no-op"}});
 assert(prepared_partition().empty()&&calls.empty());
 candidates={"/dev/null"};calls.push_back({{helper,"--prepare"},{0,""}});
 calls.push_back({{helper,"--verify","/dev/null"},{0,""}});signature();
 assert(prepared_partition()=="/dev/null"&&calls.empty());
}
CPP
for compiler in g++ clang++;do
 for mode in plain sanitized;do
  flags=(-std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror)
  [[ $mode != sanitized ]] || flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  "$compiler" "${flags[@]}" "$work/test.cpp" -o "$work/check"
  readelf -h "$work/check" | grep -F AArch64
  timeout 30s "$work/check"
  echo 'PASS: actual super swap partition admission, legacy physical path, helper ownership, signature and refusal before file fallback; synthetic only'
 done
done
