#!/bin/bash
# COPR-only native IPC/lifetime control; no GPU or image acceptance.
set -Eeuo pipefail
root=$(realpath "${1:?prepared backend source}")
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
cat > "$work/check.cpp" <<'CPP'
#include "uke-video-activity.hpp"
#include "uke-video-protocol.hpp"
#include <systemd/sd-bus.h>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <unistd.h>
struct Seen { unsigned active=0, idle=0; std::uint64_t stream=0; };
static int report(sd_bus_message *m,void *data,sd_bus_error *) {
 auto &seen=*static_cast<Seen *>(data);std::uint64_t id=0;std::uint32_t w=0,h=0;int active=0;
 assert(sd_bus_message_read(m,"tuub",&id,&w,&h,&active)>0);
 assert(id && w==3840 && h==2160);if(seen.stream)assert(id==seen.stream);seen.stream=id;
 if(active)++seen.active;else ++seen.idle;
 return sd_bus_reply_method_return(m,"");
}
int main() {
 sd_bus *bus=nullptr;assert(sd_bus_open_user(&bus)>=0);Seen seen;
 const sd_bus_vtable v[]={SD_BUS_VTABLE_START(0),SD_BUS_METHOD("Report","tuub","",report,SD_BUS_VTABLE_UNPRIVILEGED),SD_BUS_VTABLE_END};
 assert(sd_bus_add_object_vtable(bus,nullptr,uke::video::path,uke::video::name,v,&seen)>=0);
 assert(sd_bus_request_name(bus,uke::video::name,0)>=0);
 auto activity=std::make_unique<irisva::VideoActivity>(3840,2160);
 const auto start=std::chrono::steady_clock::now();
 while(std::chrono::steady_clock::now()-start<std::chrono::seconds(6)) {
  if(seen.active<2)activity->touch();
  while(sd_bus_process(bus,nullptr)>0){}
  if(seen.active>=2 && seen.idle)break;
  sd_bus_wait(bus,20000);
 }
 assert(seen.active>=2 && seen.active<=12 && seen.idle==1);
 activity.reset();sd_bus_flush_close_unref(bus);
}
CPP
read -ra flags <<< "$(pkg-config --cflags --libs libsystemd)"
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -pthread -I"$root/src" "$work/check.cpp" "$root/src/uke-video-activity.cpp" "${flags[@]}" -o "$work/check"
dbus-run-session -- "$work/check"
printf '%s\n' 'Asynchronous UHD activity IPC, coalescing, idle and lifetime control: passed (no GPU)'
