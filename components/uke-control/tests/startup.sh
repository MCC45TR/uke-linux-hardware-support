#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Execute the actual boot coordinator with independent task failures in private files.
set -Eeuo pipefail
project=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
awk '/^void boot\(\)/{copy=1} /^std::string report\(\)/{copy=0} copy' "$project/src/backend.cpp" > "$work/boot.inc"
cat > "$work/test.cpp" <<'CPP'
#include "charge-intent.hpp"
#include "sysfs.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <sys/stat.h>
namespace fs=std::filesystem;
namespace uke {
static fs::path state;static unsigned failures=0,attempted=0;
static void need(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
static void root(){}static void board(){}struct Lock{~Lock(){}};
static void directory(const fs::path& path){fs::create_directories(path);}
static std::string inventory(bool){attempted|=1;need(!(failures&1),"inventory failure");return "{}";}
static void swap_apply(){attempted|=2;need(!(failures&2),"swap failure");}
static std::string private_charge_record(const fs::path& path){std::ifstream input(path);return {std::istreambuf_iterator<char>(input),{}};}
static unsigned decimal(const std::string& value){return unsigned(std::stoul(value));}
static void charge_set_locked(unsigned value){assert(value==80);attempted|=4;need(!(failures&4),"charge failure");}
static void atomic_file(const fs::path& path,const std::string& value){std::ofstream(path)<<value;assert(!chmod(path.c_str(),0600));}
#include "boot.inc"
}
int main(int argc,char** argv) {
    assert(argc==2);using namespace uke;state=argv[1];directory(state);std::ofstream(state/"charge-limit")<<"80\n";
    for(failures=0;failures<8;++failures) {
        attempted=0;bool rejected=false;try{boot();}catch(const std::exception&){rejected=true;}
        assert(attempted==7&&rejected==bool(failures));
        const auto receipt=private_charge_record(state/"startup.json");
        for(const auto& [id,mask]:{std::pair{"inventory",1U},std::pair{"swap",2U},std::pair{"legacy_charge",4U}})
            assert(receipt.find(std::string("\"")+id+"\":{\"completed\":"+((failures&mask)?"false":"true"))!=receipt.npos);
    }
    failures=0;attempted=0;std::ofstream(state/"charge-intent")<<charge::intent::qualified("charge80");boot();assert(attempted==3);
    assert(private_charge_record(state/"startup.json").find("superseded_by_qualified_intent")!=std::string::npos);
}
CPP
${CXX:-c++} -std=c++20 -Wall -Wextra -Werror -UNDEBUG -I"$project/src" -I"$work" "$work/test.cpp" -o "$work/check"
"$work/check" "$work/state"
printf '%s\n' 'PASS: all eight startup task outcomes, durable receipt and retired legacy charge intent'
