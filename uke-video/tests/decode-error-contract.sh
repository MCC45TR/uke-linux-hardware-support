#!/usr/bin/env bash
# Exercise the actual damaged-picture and sync functions; no device or GPU.
set -Eeuo pipefail
root=$(realpath "${1:?Pass the prepared backend source}")
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
cat > "$work/check.cpp" <<'CPP'
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <mutex>
#include <poll.h>
#include <stdexcept>
#include <vector>
using VAStatus=int;
constexpr VAStatus VA_STATUS_SUCCESS=0, VA_STATUS_ERROR_DECODING_ERROR=1,
 VA_STATUS_ERROR_ALLOCATION_FAILED=2, VA_STATUS_ERROR_OPERATION_FAILED=3,
 VA_STATUS_ERROR_TIMEDOUT=4;
struct Error:std::runtime_error { VAStatus status;
 Error(VAStatus s,const char* message):std::runtime_error(message),status(s){} };
static void check(bool ok,const char* text,VAStatus status=VA_STATUS_ERROR_OPERATION_FAILED) {
 if(!ok)throw Error(status,text);
}
template<class... Args> static void trace(const char*,Args...) {}
struct Memory { bool filled=false,queued=false; };
struct Surface {
 VAStatus status=VA_STATUS_SUCCESS; bool persistent_export=false,pending=true;
 unsigned width=320,height=240;uint64_t token=7;std::shared_ptr<Memory> memory;
};
static int allocation_failure;static unsigned allocations;
static std::shared_ptr<Memory> allocate_surface(int,unsigned,unsigned,unsigned) {
 ++allocations;if(allocation_failure==1)throw Error(VA_STATUS_ERROR_ALLOCATION_FAILED,"allocation");
 if(allocation_failure==2)throw std::bad_alloc();
 return std::make_shared<Memory>();
}
static void fill_neutral_surface(Memory& memory,unsigned,unsigned) {memory.filled=true;}
static bool expired(std::chrono::steady_clock::time_point) {return false;}
struct Decoder {
 std::map<uint64_t,std::shared_ptr<Surface>> pending_;
 int render_fd_=42,fd_=-1;unsigned fourcc_=0;
 std::mutex mutex_;std::vector<std::shared_ptr<Memory>> capture_;bool last_=false;
 void pump(int){}
 void recover_picture(uint64_t token);
 void sync(const std::shared_ptr<Surface>& surface);
};
CPP
for fn in recover_picture sync; do
 awk -v fn="$fn" '$0 ~ "^void Decoder::" fn "\\(" {emit=1} emit {print} emit && /^}/ {found=1;exit} END {if(!found)exit 1}' "$root/src/decoder.cpp" >> "$work/check.cpp"
done
cat >> "$work/check.cpp" <<'CPP'
static void failed_sync(Decoder& decoder,const std::shared_ptr<Surface>& surface,VAStatus expected) {
 try {decoder.sync(surface);assert(false && "decode error must reach sync");}
 catch(const Error& e){assert(e.status==expected);}
}
int main() {
 for(bool persistent:{false,true}) {
  Decoder decoder;auto surface=std::make_shared<Surface>();
  surface->persistent_export=persistent;surface->memory=std::make_shared<Memory>();
  const auto old=surface->memory;decoder.pending_[7]=surface;const auto before=allocations;
  decoder.recover_picture(7);
  assert(surface->status==VA_STATUS_ERROR_DECODING_ERROR && !surface->pending);
  assert(decoder.pending_.empty() && surface->memory->filled);
  assert(persistent ? surface->memory==old : surface->memory!=old);
  assert(allocations==before+(persistent?0:1));
  failed_sync(decoder,surface,VA_STATUS_ERROR_DECODING_ERROR);
  decoder.recover_picture(7);assert(allocations==before+(persistent?0:1));
 }
 for(int failure:{1,2}) {
  Decoder decoder;auto surface=std::make_shared<Surface>();decoder.pending_[7]=surface;
  allocation_failure=failure;decoder.recover_picture(7);
  assert(!surface->pending && decoder.pending_.empty());
  failed_sync(decoder,surface,VA_STATUS_ERROR_ALLOCATION_FAILED);
 }
 Decoder decoder;const auto before=allocations;decoder.recover_picture(99);assert(allocations==before);
}
CPP
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Werror "$work/check.cpp" -o "$work/check"
"$work/check"
printf 'Actual damaged-frame recovery/sync: decode failure retained, exported storage preserved, allocation failure and duplicate completion passed (host only)\n'
