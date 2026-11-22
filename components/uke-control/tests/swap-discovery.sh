#!/usr/bin/env bash
# Actual-source candidate discovery; mock commands, no block-device writes.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
cat > "$work/test.cpp" <<'CPP'
#include <cassert>
#include <deque>
#include <filesystem>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs=std::filesystem;
struct Result {int status; std::string output;};
static void need(bool value,const std::string& message) {if(!value)throw std::runtime_error(message);}
static std::deque<std::pair<std::vector<std::string>,Result>> calls;
static Result execute(const std::vector<std::string>& args) {
    assert(!calls.empty());assert(calls.front().first==args);
    auto result=calls.front().second;calls.pop_front();return result;
}
CPP
awk '/^static std::set<std::string> partition_candidates\(\)/{copy=1} copy&&/^static std::string partition\(\)/{exit} copy{print}' "$root/src/backend.cpp" >> "$work/test.cpp"
cat >> "$work/test.cpp" <<'CPP'
static void prepare(std::string gpt,std::string label={},int label_status=2) {
    calls={{{"/usr/bin/lsblk","--raw","--noheadings","--output","PATH,PARTLABEL"},{0,gpt}},
           {{"/usr/bin/blkid","-t","LABEL=linux_swap","-o","device"},{label_status,label}}};
}
static void refused() {bool failed=false;try{(void)partition_candidates();}catch(const std::exception&){failed=true;}assert(failed);}
int main() {
    prepare("/dev/null \n/dev/zero other\n");assert(partition_candidates().empty());assert(calls.empty());
    prepare("/dev/null linux_swap\n");assert(partition_candidates()==std::set<std::string>{"/dev/null"});assert(calls.empty());
    prepare("/dev/null other\n","/dev/zero\n",0);assert(partition_candidates()==std::set<std::string>{"/dev/zero"});
    prepare("/dev/null linux_swap\n/dev/./null linux_swap\n","/dev/null\n",0);assert(partition_candidates().size()==1);
    prepare("/dev/null linux_swap\n","/dev/zero\n",0);refused();
    prepare("/dev/null linux_swap\\x20other\n");assert(partition_candidates().empty());
    prepare("/dev/null linux_swap extra\n");refused();
    prepare("relative linux_swap\n");refused();
    prepare("/dev/null \n","relative\n",0);refused();
    prepare("/dev/null \n");calls.front().second.status=1;refused();
    prepare("/dev/null \n",{},4);refused();
    prepare("/dev/uke-nonexistent-fixture linux_swap\n");refused();
}
CPP
"${CXX:-c++}" -std=c++20 -O2 -Wall -Wextra -Werror "$work/test.cpp" -o "$work/test"
"$work/test"
printf '%s\n' 'Actual swap discovery: GPT/filesystem labels, aliases, ambiguity, escaped fields and failures passed; no block writes'
