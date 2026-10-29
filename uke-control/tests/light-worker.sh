#!/usr/bin/env bash
# Compile the actual worker with host-only IO stubs; no device command.
set -Eeuo pipefail
cd "$(dirname "$0")/.."
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
cat > "$stage/test.cpp" <<'CPP'
#include "lights.hpp"
#include <cassert>
#include <csignal>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <unistd.h>
#include <vector>
namespace uke::lights {
static std::filesystem::path location;
static std::vector<std::string> writes;
static unsigned calls, mode, owned;
static void need(bool ok,const std::string& text){if(!ok)throw std::runtime_error(text);}
static void root() {}
static Source required(bool){Source s;s.path=location;s.brightness=owned;return s;}
static void write_fd(int fd,const std::string& value){assert(fd>=0);writes.push_back(value);}
static std::string raw(const std::string&,const std::filesystem::path&){return mode==1?"0":writes.back().substr(0,1);}
static int sd_notify(int,const char*){if(mode==3)std::raise(SIGTERM);return mode==2?-1:0;}
static volatile sig_atomic_t stopped;
static void stop(int){stopped=1;}
static unsigned read_target() {
    ++calls;
    if(mode==4 && calls==2)throw std::runtime_error("Invalid target");
    return calls==1?1:calls<1002?2:0;
}
static int test_poll(pollfd* fds,nfds_t count,int milliseconds) {
    assert(!fds && !count && milliseconds==100);return 0;
}
#define poll test_poll
CPP
sed -n '/^struct Fd /p' src/lights.cpp >> "$stage/test.cpp"
awk '/^static void hold\(\)/ {copy=1} copy {print} copy && /^}/ {exit}' src/lights.cpp >> "$stage/test.cpp"
cat >> "$stage/test.cpp" <<'CPP'
namespace storage {
static std::filesystem::path file;
struct Lock {Fd fd;explicit Lock(bool,bool=false):fd{open(file.c_str(),O_CREAT|O_RDWR,0600)} {assert(fd.n>=0);}};
CPP
for function in save_target read_target; do
    awk -v name="$function" '$0 ~ "^static .* "name"\\(" {copy=1} copy {print} copy && /^}/ {exit}' src/lights.cpp >> "$stage/test.cpp"
done
cat >> "$stage/test.cpp" <<'CPP'
}
}
int main(int argc,char** argv) {
    using namespace uke::lights;
    assert(argc==2);location=argv[1];std::ofstream(location/"brightness")<<"0\n";
    storage::file=location/"request";
    for(unsigned n=0;n<=2;++n){storage::save_target(n);assert(storage::read_target()==n);}
    for(const std::string value:{"","00","3","-1","1\n"}) {
        std::ofstream(storage::file)<<value;bool failed=false;
        try{(void)storage::read_target();}catch(...){failed=true;}assert(failed);
    }
    for(mode=0;mode<5;++mode) {
        writes.clear();calls=0;stopped=0;owned=0;
        bool failed=false;try{hold();}catch(...){failed=true;}
        assert(failed==(mode==1 || mode==2 || mode==4));
        if(mode==0) {
            assert(calls==1002); // More than 60 seconds of 100 ms polls; no forced deadline.
            assert((writes==std::vector<std::string>{"1\n","2\n","0\n","0\n"}));
        } else assert((writes==std::vector<std::string>{"1\n","0\n"}));
    }
    writes.clear();owned=2;bool failed=false;try{hold();}catch(...){failed=true;}
    assert(failed && writes.empty());
    std::cout<<"Actual torch worker: level changes, continued operation, explicit zero, SIGTERM, readback/notify/target failure cleanup and ownership refusal passed; simulated IO only\n";
}
CPP
${CXX:-c++} -std=c++20 -Wall -Wextra -Werror -UNDEBUG -Isrc "$stage/test.cpp" -o "$stage/test"
"$stage/test" "$stage"
