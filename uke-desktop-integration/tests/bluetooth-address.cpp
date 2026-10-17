// SPDX-License-Identifier: GPL-3.0-or-later
#define UKE_BLUETOOTH_TEST
#include "../src/bluetooth-address.cpp"
#include <cassert>
template<class F> static void refuses(F f) { bool refused=false;try{f();}catch(const std::exception&){refused=true;}assert(refused); }
static void packet(int fd,unsigned event,unsigned index,const Bytes &payload) {
    Bytes bytes;append16(bytes,event);append16(bytes,index);append16(bytes,payload.size());
    bytes.insert(bytes.end(),payload.begin(),payload.end());
    assert(send(fd,bytes.data(),bytes.size(),0)==static_cast<ssize_t>(bytes.size()));
}
static void response(int fd,unsigned opcode,unsigned status,const Bytes &data={}) {
    Bytes bytes;append16(bytes,opcode);bytes.push_back(status);bytes.insert(bytes.end(),data.begin(),data.end());
    packet(fd,1,0,bytes);
}
int main() {
    const Bytes input{0x12,0x34,0x56,0x78,0x9a,0xbc},address{0xbc,0x9a,0x78,0x56,0x34,0x12};
    assert(factory_address(input)==address);
    for(const auto &bad:{Bytes{},Bytes(5,1),Bytes(7,1),Bytes(6,0),Bytes(6,255),Bytes{0,0,0,0,0x5a,0xad}})
        refuses([&]{factory_address(bad);});
    assert(compatible(Bytes{'x',0,'u','k','e',0},"uke"));
    assert(!compatible(Bytes{'x',0},"uke"));
    refuses([]{compatible(Bytes{'u','k','e'},"uke");});
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    int fd[2];assert(socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0,fd)==0);
    response(fd[1],0x37,0x11);packet(fd[1],0x1d,1,{});packet(fd[1],0x1d,0,{});
    response(fd[1],4,0x11); // Readiness during a command must not be lost.
    const Bytes missing_address{1,0,2,0,0,0,2,0,0,0};
    response(fd[1],0x37,0,missing_address);response(fd[1],0x39,0);
    configure(fd[0],address,deadline);
    for(auto opcode:{0x37,4,0x37,0x39}) {
        auto sent=receive(fd[1],deadline);assert(sent.event==unsigned(opcode)&&sent.index==0);
        assert(sent.data==(opcode==0x39 ? address : Bytes{}));
    }
    response(fd[1],0x37,0,Bytes(10,0));response(fd[1],4,0,address);
    configure(fd[0],address,deadline); // Configured controller is not rewritten.
    assert(receive(fd[1],deadline).event==0x37);assert(receive(fd[1],deadline).event==4);
    response(fd[1],0x37,0x11);response(fd[1],4,0,Bytes(6,1));
    refuses([&]{configure(fd[0],address,deadline);});
    assert(receive(fd[1],deadline).event==0x37);assert(receive(fd[1],deadline).event==4);
    auto unsupported=missing_address;unsupported[6]=4;
    response(fd[1],0x37,0,unsupported);refuses([&]{configure(fd[0],address,deadline);});
    assert(receive(fd[1],deadline).event==0x37);
    const Bytes malformed{1,0,0,0,1,0};assert(send(fd[1],malformed.data(),malformed.size(),0)==6);
    refuses([&]{receive(fd[0],deadline);});
    refuses([&]{receive(fd[0],std::chrono::steady_clock::now());});
    close(fd[0]);close(fd[1]);
    char temporary[]="/tmp/uke-bt-contract-XXXXXX";auto directory=mkdtemp(temporary);assert(directory);
    auto file=std::filesystem::path(directory)/"factory";
    {std::ofstream output(file,std::ios::binary);output.write(reinterpret_cast<const char *>(input.data()),input.size());}
    assert(file_bytes(file,6)==input);refuses([&]{file_bytes(file,5);});
    std::filesystem::remove_all(directory);
    std::cout<<"Bluetooth factory input, event readiness, management bytes, idempotence and refusal fixtures passed\n";
}
