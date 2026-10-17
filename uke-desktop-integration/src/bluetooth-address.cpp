// SPDX-License-Identifier: GPL-3.0-or-later
// Supply the exact Uke factory address through the standard BlueZ management API.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <poll.h>
#include <span>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <vector>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
using Bytes = std::vector<std::uint8_t>;
using Deadline = std::chrono::steady_clock::time_point;
static void check(bool ok,const char *message) { if(!ok)throw std::runtime_error(message); }
static Bytes file_bytes(const std::filesystem::path &path,std::size_t maximum) {
    std::ifstream file(path,std::ios::binary);
    check(bool(file),"Bluetooth factory input unavailable");
    Bytes bytes;
    char byte;
    while(file.get(byte)) {
        check(bytes.size()<maximum,"Bluetooth factory input exceeds bounds");
        bytes.push_back(static_cast<std::uint8_t>(byte));
    }
    check(file.eof()&&!file.bad(),"Bluetooth factory input read failed");
    return bytes;
}
static bool compatible(const Bytes &bytes,const std::string &expected) {
    std::size_t offset=0;
    bool match=false;
    while(offset<bytes.size()) {
        auto end=std::find(bytes.begin()+offset,bytes.end(),0);
        check(end!=bytes.end(),"Invalid Bluetooth device identity");
        match|=std::string(bytes.begin()+offset,end)==expected;
        offset=std::distance(bytes.begin(),end)+1;
    }
    return match;
}
static Bytes factory_address(const Bytes &input) {
    check(input.size()==6,"Uke Bluetooth factory address must contain six bytes");
    check(!std::all_of(input.begin(),input.end(),[](auto b){return b==0;}) &&
          !std::all_of(input.begin(),input.end(),[](auto b){return b==255;}),"Invalid factory Bluetooth address");
    Bytes address(input.rbegin(),input.rend()); // BlueZ bdaddr_t uses least significant byte first.
    check(address!=Bytes({0xad,0x5a,0,0,0,0}),"Default Bluetooth address is not a factory identity");
    return address;
}
static unsigned le16(std::span<const std::uint8_t> bytes) { return bytes[0]|unsigned(bytes[1])<<8; }
static void append16(Bytes &bytes,unsigned value) { bytes.push_back(value);bytes.push_back(value>>8); }
struct Packet { unsigned event,index; Bytes data; };
static Packet receive(int fd,Deadline deadline) {
    const auto left=std::chrono::duration_cast<std::chrono::milliseconds>(deadline-std::chrono::steady_clock::now()).count();
    check(left>0,"Bluetooth controller readiness timed out");
    pollfd pending{fd,POLLIN,0};
    check(poll(&pending,1,left)>0 && (pending.revents&POLLIN),"Bluetooth management event unavailable");
    std::array<std::uint8_t,4096> buffer{};
    const auto size=recv(fd,buffer.data(),buffer.size(),MSG_TRUNC);
    check(size>=6 && size<=static_cast<ssize_t>(buffer.size()) && le16(std::span(buffer).subspan(4,2))==size-6,
          "Invalid Bluetooth management packet");
    return {le16(buffer),le16(std::span(buffer).subspan(2,2)),Bytes(buffer.begin()+6,buffer.begin()+size)};
}
struct Reply { unsigned status; Bytes data; };
static Reply command(int fd,unsigned opcode,std::span<const std::uint8_t> payload,Deadline deadline,bool &index_changed) {
    Bytes request;append16(request,opcode);append16(request,0);append16(request,payload.size());
    request.insert(request.end(),payload.begin(),payload.end());
    check(send(fd,request.data(),request.size(),MSG_NOSIGNAL)==static_cast<ssize_t>(request.size()),
          "Bluetooth management command failed");
    for(;;) {
        auto packet=receive(fd,deadline);
        if(packet.index==0 && (packet.event==0x0004 || packet.event==0x001d))index_changed=true;
        if(packet.index!=0 || (packet.event!=1 && packet.event!=2))continue;
        check(packet.data.size()>=3,"Truncated Bluetooth command response");
        if(le16(packet.data)!=opcode)continue;
        return {packet.data[2],Bytes(packet.data.begin()+3,packet.data.end())};
    }
}
static void configure(int fd,const Bytes &address,Deadline deadline) {
    for(;;) {
        bool index_changed=false;
        auto config=command(fd,0x0037,{},deadline,index_changed); // Read Controller Configuration Information.
        if(config.status==0) {
            check(config.data.size()==10,"Invalid Bluetooth configuration information");
            const auto missing=config.data[6]|unsigned(config.data[7])<<8|unsigned(config.data[8])<<16|unsigned(config.data[9])<<24;
            if(missing==2) {
                check(command(fd,0x0039,address,deadline,index_changed).status==0,"Bluetooth factory address was refused");
                return;
            }
            check(missing==0,"Bluetooth controller has unexpected configuration requirements");
        }
        else check(config.status==0x11 || config.status==0x0a,"Bluetooth configuration query failed");
        auto info=command(fd,0x0004,{},deadline,index_changed); // An already configured controller is idempotent.
        if(info.status==0) {
            check(info.data.size()>=6 && std::equal(address.begin(),address.end(),info.data.begin()),
                  "Configured Bluetooth address differs from factory input");
            return;
        }
        check(info.status==0x11 || info.status==0x0a,"Bluetooth information query failed");
        if(index_changed)continue;
        for(;;) {
            auto event=receive(fd,deadline);
            if(event.index==0 && (event.event==0x0004 || event.event==0x001d))break;
        }
    }
}
#ifndef UKE_BLUETOOTH_TEST
int main() {
    int fd=-1;
    try {
        check(compatible(file_bytes("/sys/firmware/devicetree/base/compatible",4096),"xiaomi,uke") &&
              compatible(file_bytes("/sys/class/bluetooth/hci0/device/of_node/compatible",4096),"xiaomi,uke-wcn6750-bt"),
              "Factory Bluetooth setup is restricted to the exact Uke controller");
        struct statvfs mount{};
        check(statvfs("/mnt/vendor/persist",&mount)==0 && (mount.f_flag&ST_RDONLY),"Factory persist must be read-only");
        auto address=factory_address(file_bytes("/mnt/vendor/persist/bluetooth/.bt_nv.bin",6));
        fd=socket(AF_BLUETOOTH,SOCK_RAW|SOCK_CLOEXEC,BTPROTO_HCI);
        check(fd>=0,"Bluetooth management socket unavailable");
        sockaddr_hci control{};control.hci_family=AF_BLUETOOTH;control.hci_dev=HCI_DEV_NONE;control.hci_channel=HCI_CHANNEL_CONTROL;
        check(bind(fd,reinterpret_cast<sockaddr *>(&control),sizeof(control))==0,"Bluetooth management bind failed");
        configure(fd,address,std::chrono::steady_clock::now()+std::chrono::seconds(30));
        close(fd);
        std::cout<<"Uke Bluetooth factory address admitted\n";
        return 0;
    } catch(const std::exception &error) {
        if(fd>=0)close(fd);
        std::cerr<<error.what()<<'\n';return 1;
    }
}
#endif
