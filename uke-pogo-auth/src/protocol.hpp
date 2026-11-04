// SPDX-License-Identifier: MIT
#pragma once
// Original implementation of the bounded kernel handoff and object ABI.
// Wire facts: pinned QCOMTEE/MINK interfaces and the documented WN8030 exchange.
#include "../../uke-cdsp-pogo-firmware/src/auth-image.hpp"
#include <MinkCom.h>
#include <openssl/crypto.h>
#include <charconv>
#include <chrono>
#include <optional>
#include <span>
#include <string_view>
namespace uke_pogo_auth {
template<size_t N> struct Secret {
    std::array<unsigned char,N> bytes{};
    ~Secret(){OPENSSL_cleanse(bytes.data(),bytes.size());}
};
struct Request {
    uint32_t epoch=0;
    Secret<16> uid,challenge,host;
    bool operator==(const Request &other)const {
        return epoch==other.epoch&&uid.bytes==other.uid.bytes&&challenge.bytes==other.challenge.bytes&&host.bytes==other.host.bytes;
    }
};
static inline unsigned nibble(char c) {
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    if(c>='A'&&c<='F')return c-'A'+10;
    throw std::runtime_error("Invalid kernel authentication encoding");
}
static inline void decode(std::string_view text,Secret<16> &out) {
    need(text.size()==32,"Invalid kernel authentication field length");
    for(size_t i=0;i<16;i++)out.bytes[i]=nibble(text[2*i])*16+nibble(text[2*i+1]);
}
static inline Request parse(std::string_view text) {
    need(!text.empty()&&text.size()<=160,"Invalid kernel authentication request size");
    const auto whitespace=[](char c){return c==' '||c=='\t'||c=='\n';};
    std::array<std::string_view,5> fields{};size_t p=0;
    for(auto &field:fields) {
        while(p<text.size()&&whitespace(text[p]))p++;
        const auto begin=p;while(p<text.size()&&!whitespace(text[p]))p++;
        need(p>begin,"Incomplete kernel authentication request");field=text.substr(begin,p-begin);
    }
    while(p<text.size()&&whitespace(text[p]))p++;
    need(p==text.size()&&fields[0]=="1","Unknown kernel authentication ABI or trailing data");
    Request out;const auto result=std::from_chars(fields[1].data(),fields[1].data()+fields[1].size(),out.epoch);
    need(result.ec==std::errc{}&&result.ptr==fields[1].data()+fields[1].size(),"Invalid kernel authentication epoch");
    decode(fields[2],out.uid);decode(fields[3],out.challenge);decode(fields[4],out.host);return out;
}
static inline void put32(Secret<128> &buffer,size_t at,uint32_t value) {
    for(unsigned i=0;i<4;i++)buffer.bytes.at(at+i)=value>>(8*i);
}
static inline Secret<128> command(const Request &request) {
    Secret<128> out;put32(out,0,1);put32(out,4,0x2020);
    constexpr char digits[]="0123456789ABCDEF";
    for(size_t i=0;i<16;i++){out.bytes[12+2*i]=digits[request.uid.bytes[i]>>4];out.bytes[13+2*i]=digits[request.uid.bytes[i]&15];}
    constexpr char key_metadata[]="00000002";
    std::copy_n(key_metadata,8,out.bytes.begin()+44);
    std::copy(request.challenge.bytes.begin(),request.challenge.bytes.end(),out.bytes.begin()+52);return out;
}
class ObjectRef {
    Object object_=Object_NULL;
public:
    ObjectRef()=default;
    ~ObjectRef(){if(!Object_isNull(object_))(void)Object_release(object_);}
    ObjectRef(const ObjectRef&)=delete;ObjectRef& operator=(const ObjectRef&)=delete;
    Object get()const{return object_;}
    Object *output(){return &object_;}
    void adopt(Object object){need(Object_isNull(object_),"Authentication object already owned");object_=object;}
    void require()const{need(!Object_isNull(object_),"TEE returned no authentication object");}
};
class TeeTransport {
    // Destruction releases the app before loader/client/root and callback workers.
    ObjectRef root_,client_,loader_,app_;
public:
    explicit TeeTransport(std::span<const unsigned char> admitted_image) {
        need(admitted_image.size()==uke_local_fw::auth_bytes,"Wrong authentication image size");
        const int root_status=MinkCom_getRootEnvObject(root_.output());
        need(root_status==0,"Cannot obtain the QTEE root environment");root_.require();
        const int client_status=MinkCom_getClientEnvObject(root_.get(),client_.output());
        need(client_status==0,"Cannot register the QTEE authentication client");client_.require();
        uint32_t loader_uid=122;ObjectArg open[2]{};
        open[0].bi={&loader_uid,sizeof(loader_uid)};open[1].o=Object_NULL;
        const int open_status=Object_invoke(client_.get(),0,open,ObjectCounts_pack(1,0,0,1));
        loader_.adopt(open[1].o);need(open_status==0,"Cannot obtain the QSEE-compatible app loader");loader_.require();
        constexpr char filename[]="devauth";Secret<128> distribution;
        ObjectArg load[4]{};load[0].bi={admitted_image.data(),admitted_image.size()};
        load[1].bi={filename,sizeof(filename)};load[2].b={distribution.bytes.data(),distribution.bytes.size()};load[3].o=Object_NULL;
        const int load_status=Object_invoke(loader_.get(),1,load,ObjectCounts_pack(2,1,0,1));
        app_.adopt(load[3].o);
        need(load_status==0&&load[2].b.size<=distribution.bytes.size(),"QTEE refused the exact Uke authentication image");app_.require();
    }
    Secret<16> issue(const Request &request) {
        auto input=command(request);Secret<128> initial_response,request_output,response;
        response.bytes.fill(0xff);uint32_t is64=1;ObjectArg args[10]{};
        args[0].bi={input.bytes.data(),input.bytes.size()};
        args[1].bi={initial_response.bytes.data(),initial_response.bytes.size()};
        args[2].bi={nullptr,0};args[3].bi={&is64,sizeof(is64)};
        args[4].b={request_output.bytes.data(),request_output.bytes.size()};
        args[5].b={response.bytes.data(),response.bytes.size()};
        for(size_t i=6;i<10;i++)args[i].o=Object_NULL;
        const int status=Object_invoke(app_.get(),0,args,ObjectCounts_pack(4,2,4,0));
        need(status==0,"Authentication TEE call failed; no token was submitted");
        need(args[4].b.ptr==request_output.bytes.data()&&args[5].b.ptr==response.bytes.data()&&
             args[4].b.size<=128&&args[5].b.size>=24&&args[5].b.size<=128,"Truncated, replaced or oversized authentication reply");
        const auto result=uint32_t(response.bytes[0])|uint32_t(response.bytes[1])<<8|
                          uint32_t(response.bytes[2])<<16|uint32_t(response.bytes[3])<<24;
        need(result==0,"Authentication TA returned an error; no token was submitted");
        Secret<16> token;std::copy_n(response.bytes.begin()+8,16,token.bytes.begin());
        need(std::any_of(token.bytes.begin(),token.bytes.end(),[](auto b){return b!=0;})&&
             std::any_of(token.bytes.begin(),token.bytes.end(),[](auto b){return b!=0xff;}),
             "Authentication TA returned no usable token");
        return token;
    }
};
template<class Backend,class Bridge,class Clock>
bool process(const Request &request,Backend &backend,Bridge &bridge,Clock now) {
    const auto start=now();auto token=backend.issue(request);
    if(now()-start>=std::chrono::seconds(4))return false;
    const auto current=bridge.current();
    if(!current||!(*current==request)||now()-start>=std::chrono::seconds(4))return false;
    return bridge.submit(request.epoch,token);
}
} // namespace uke_pogo_auth
