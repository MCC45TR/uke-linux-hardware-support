#!/usr/bin/env bash
# Actual backend routing with host-only stubs; no board, helper or service access.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -r -- "$work"' EXIT
{
cat <<'CPP'
#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>
struct Result { int status; std::string output; };
static unsigned checked;
static std::vector<std::string> sent;
static void root(){checked++;}
static void board(){checked++;}
struct Lock { Lock(){checked++;} };
static void charge_helper(){checked++;}
static void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
static Result execute(const std::vector<std::string> &args,unsigned seconds,size_t limit){
 assert(checked==4&&seconds==10&&limit==65536);sent=args;return {69,"review refused"};
}
CPP
awk '/^Result charge_review\(/ {copy=1} copy{print} copy&&/^}/{exit}' "$root/src/backend.cpp"
cat <<'CPP'
int main(){
 for(const auto &path:{std::string(""),std::string("relative.json"),std::string("/")+std::string(4096,'a')}){
  checked=0;sent.clear();bool refused=false;try{charge_review(path);}catch(...){refused=true;}
  assert(refused&&sent.empty());
 }
 checked=0;const std::string path="/tmp/review with spaces.json";
 const auto r=charge_review(path);assert(r.status==69&&r.output=="review refused");
 assert((sent==std::vector<std::string>{"/usr/libexec/uke-charge-control","identity-review",path}));
}
CPP
} > "$work/review.cpp"
"${CXX:-c++}" -std=c++20 -O2 -Wall -Wextra -Werror "$work/review.cpp" -o "$work/review"
"$work/review"
grep -Fq 'uke::charge_review(argv[3])' "$root/src/main.cpp"
grep -Fq 'uke::charge_control_mode("profile-restore")' "$root/src/main.cpp"
echo 'Actual review path and explicit restore routing passed with host stubs; no device operation'
