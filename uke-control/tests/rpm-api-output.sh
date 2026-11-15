#!/usr/bin/bash
# COPR-native integration of the actual bounded collector, RPM and API parser.
set -Eeuo pipefail
[[ $# == 2 ]]
root=$(realpath "$1"); parent=$(realpath "$2")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
[[ $(grep -c 'PROVIDENAME.*},5,65536);' "$root/src/backend.cpp") == 1 ]]
[[ $(grep -c 'PROVIDENAME.*},5,65536);' "$root/src/usb-request.cpp") == 2 ]]
! grep -q 'PROVIDENAME.*},5,4096);' "$root/src/backend.cpp" "$root/src/usb-request.cpp"
awk '/^ProcessResult execute_bounded\(/{copy=1} /^Result execute\(/{copy=0} copy' "$root/src/backend.cpp" > "$work/collector.inc"
[[ -s $work/collector.inc ]]
cat > "$work/check.cpp" <<'CPP'
#include "backend.hpp"
#include "rpm-api.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
namespace uke {
static void need(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
struct Fd {int n;~Fd(){if(n>=0)close(n);}};
#include "collector.inc"
}
int main(int argc,char** argv) {
    assert(argc==2);
    const std::vector<std::string> query={"/usr/bin/rpm","-qp","--qf","[%{PROVIDENAME}\\t%{PROVIDEVERSION}\\n]",argv[1]};
    auto old=uke::execute_bounded(query,5,4096);
    assert(old.status==124 && old.truncated && !old.timed_out);
    auto current=uke::execute_bounded(query,5,65536);
    assert(current.status==0 && !current.truncated && !current.timed_out && current.output.size()>4096);
    for(auto name:{"uke-usb-manual-cdc-api","uke-usb-gadget-ownership-api"}) {
        assert(uke::rpm_api_matches(current.output,name,"2"));
        assert(!uke::rpm_api_matches(current.output,name,"1"));
        assert(!uke::rpm_api_matches(current.output,name,"21"));
        assert(!uke::rpm_api_matches(current.output+name+std::string("\t2\n"),name,"2"));
        assert(!uke::rpm_api_matches(current.output.substr(0,current.output.size()-1),name,"2"));
    }
    auto exact=uke::execute_bounded({"/usr/bin/printf","%65536s","x"},5,65536);
    assert(exact.status==0 && exact.output.size()==65536 && !exact.truncated);
    auto excess=uke::execute_bounded({"/usr/bin/printf","%65537s","x"},5,65536);
    assert(excess.status==124 && excess.truncated && !excess.timed_out);
}
CPP
for compiler in g++ clang++;do
 for mode in plain sanitize;do
  flags=();[[ $mode != sanitize ]] || flags=(-fsanitize=address,undefined -fno-sanitize-recover=all)
  "$compiler" -std=c++20 -O1 -g -Wall -Wextra -Werror "${flags[@]}" -I "$root/src" -I "$work" "$work/check.cpp" -o "$work/check"
  "$work/check" "$parent"
 done
done
printf 'Actual bounded collector and RPM/API integration passed: signed20 metadata exceeds old4096 limit; both API2 checks and truncation/duplicate/version refusals preserved.\n'
