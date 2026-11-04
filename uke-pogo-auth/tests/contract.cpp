// SPDX-License-Identifier: MIT
#define UKE_LOCAL_FW_TEST
#define UKE_KERNEL_UPDATE_TEST
#include "../src/protocol.hpp"
#include <cassert>
using namespace uke_pogo_auth;
namespace {
struct Node{unsigned role;};
std::array<Node,4> nodes{{{0},{1},{2},{3}}};
std::vector<unsigned> released;
int fail_stage=0,reply_case=0,sends=0;
int32_t invoke(ObjectCxt context,ObjectOp operation,ObjectArg *args,ObjectCounts counts);
Object object(unsigned role){return {invoke,&nodes.at(role)};}
int32_t invoke(ObjectCxt context,ObjectOp operation,ObjectArg *args,ObjectCounts counts) {
    const auto role=static_cast<Node*>(context)->role;
    if(operation==Object_OP_release){released.push_back(role);return 0;}
    if(role==1) {
        assert(operation==0&&counts==ObjectCounts_pack(1,0,0,1));
        assert(args[0].bi.size==4&&*static_cast<const uint32_t*>(args[0].bi.ptr)==122);
        args[1].o=object(2);return fail_stage==3?-1:0;
    }
    if(role==2) {
        assert(operation==1&&counts==ObjectCounts_pack(2,1,0,1));
        assert(args[0].bi.size==uke_local_fw::auth_bytes);
        assert(args[1].bi.size==8&&!memcmp(args[1].bi.ptr,"devauth\0",8));
        assert(args[2].b.size==128);args[2].b.size=0;
        args[3].o=object(3);return fail_stage==4?-1:0;
    }
    assert(role==3&&operation==0&&counts==ObjectCounts_pack(4,2,4,0));sends++;
    assert(args[0].bi.size==128&&args[1].bi.size==128&&args[2].bi.size==0);
    assert(args[3].bi.size==4&&*static_cast<const uint32_t*>(args[3].bi.ptr)==1);
    for(unsigned i=6;i<10;i++)assert(Object_isNull(args[i].o));
    const auto *request=static_cast<const unsigned char*>(args[0].bi.ptr);
    assert(request[0]==1&&request[4]==0x20&&request[5]==0x20&&request[8]==0);
    assert(!memcmp(request+12,"ABABABABABABABABABABABABABABABAB",32));
    assert(!memcmp(request+44,"00000002",8));
    for(unsigned i=0;i<16;i++)assert(request[52+i]==0xcd);
    auto *reply=static_cast<unsigned char*>(args[5].b.ptr);
    if(reply_case!=9) {
        memset(reply,0,128);for(unsigned i=0;i<16;i++)reply[8+i]=i+1;
    }
    switch(reply_case) {
    case 1:return -1;
    case 2:reply[0]=0x0d;reply[3]=0xaa;break;
    case 3:args[5].b.size=23;break;
    case 4:args[5].b.size=129;break;
    case 5:args[4].b.size=129;break;
    case 6:memset(reply+8,0,16);break;
    case 7:memset(reply+8,0xff,16);break;
    case 8:args[5].b.ptr=nullptr;break;
    default:break;
    }
    return 0;
}
template<class F>void refused(F operation) {
    bool failed=false;try{operation();}catch(const std::exception&){failed=true;}assert(failed);
}
struct Backend {
    bool fail=false;
    Secret<16> issue(const Request&){if(fail)throw std::runtime_error("Synthetic TA failure");Secret<16> token;token.bytes.fill(42);return token;}
};
struct FakeBridge {
    std::optional<Request> live;
    bool accepted=true;unsigned attempts=0;
    auto current(){return live;}
    bool submit(uint32_t epoch,const Secret<16>&token){assert(epoch==7&&token.bytes[0]==42);attempts++;return accepted;}
};
}
extern "C" int MinkCom_getRootEnvObject(Object *out){*out=object(0);return fail_stage==1?-1:0;}
extern "C" int MinkCom_getClientEnvObject(Object root,Object *out){assert(root.context==&nodes[0]);*out=object(1);return fail_stage==2?-1:0;}
int main() {
    const std::string uid(32,'a');
    const std::string wire="1 7 abababababababababababababababab cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd efefefefefefefefefefefefefefefef\n";
    const auto request=parse(wire);assert(request.epoch==7&&request.uid.bytes[0]==0xab&&request.host.bytes[0]==0xef);
    for(const auto &bad:{"2 7 "+uid+" "+uid+" "+uid,"1 -1 "+uid+" "+uid+" "+uid,
        "1 4294967296 "+uid+" "+uid+" "+uid,wire+"extra","1 7 "+uid+" "+uid,
        "1 7 z"+uid.substr(1)+" "+uid+" "+uid})refused([&]{parse(bad);});
    const Bytes image(uke_local_fw::auth_bytes);
    assert(!uke_local_fw::admitted_auth(image));
    for(int stage=1;stage<=4;stage++) {
        fail_stage=stage;released.clear();refused([&]{TeeTransport backend(image);});
        assert(released.size()==unsigned(stage));
        for(int i=0;i<stage;i++)assert(released[i]==unsigned(stage-1-i));
    }
    fail_stage=0;released.clear();
    {
        TeeTransport backend(image);auto token=backend.issue(request);
        assert(token.bytes[0]==1&&token.bytes[15]==16);
        for(reply_case=1;reply_case<=9;reply_case++)refused([&]{backend.issue(request);});
    }
    assert((released==std::vector<unsigned>{3,2,1,0})&&sends==10);
    Backend backend;FakeBridge bridge{request};
    const auto now=[] {return std::chrono::steady_clock::time_point{};};
    assert(process(request,backend,bridge,now)&&bridge.attempts==1);
    for(unsigned field=0;field<5;field++) {
        bridge.attempts=0;bridge.live=request;
        if(field==0)bridge.live.reset();
        if(field==1)bridge.live->epoch++;
        if(field==2)bridge.live->uid.bytes[0]++;
        if(field==3)bridge.live->challenge.bytes[0]++;
        if(field==4)bridge.live->host.bytes[0]++;
        assert(!process(request,backend,bridge,now)&&!bridge.attempts);
    }
    bridge.live=request;unsigned tick=0;
    assert(!process(request,backend,bridge,[&]{return now()+std::chrono::seconds(tick++?4:0);})&&!bridge.attempts);
    tick=0;assert(!process(request,backend,bridge,[&]{const std::array<unsigned,3> time{0,3999,4001};return now()+std::chrono::milliseconds(time.at(tick++));})&&!bridge.attempts);
    bridge.accepted=false;assert(!process(request,backend,bridge,now)&&bridge.attempts==1);
    bridge.attempts=0;backend.fail=true;refused([&]{process(request,backend,bridge,now);});assert(!bridge.attempts);
    std::cout<<"PASS: actual request parsing, MINK argument shapes/cleanup, failed/short/error TA replies, epoch/full-challenge/deadline checks and stale-store refusal; synthetic objects only\n";
}
