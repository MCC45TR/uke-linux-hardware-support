// SPDX-License-Identifier: MIT
#pragma once
#include "fat.hpp"
namespace uke_local_fw {
inline constexpr size_t auth_bytes=147544;
inline constexpr const char *auth_sha="ae066b0126626b446c0bcafc486180227fb5e0e52b7907b7aee05c28f056ebe4";
static inline uint64_t auth_field(const Bytes &b,size_t p,unsigned width) {
    need((width==2||width==4||width==8)&&p<=b.size()&&width<=b.size()-p,"Truncated authentication ELF field");
    uint64_t value=0;for(unsigned i=0;i<width;i++)value|=uint64_t(b[p+i])<<(i*8);return value;
}
template<class Reader> Bytes join_auth_parts(const Bytes &mdt,Reader read) {
    need(mdt.size()>=64&&mdt.size()<=1024*1024&&!memcmp(mdt.data(),"\177ELF",4)&&
         (mdt[4]==1||mdt[4]==2)&&mdt[5]==1&&mdt[6]==1&&auth_field(mdt,18,2)==183,
         "Authentication metadata is not the retained little-endian AArch64 ELF");
    const bool elf64=mdt[4]==2;
    const size_t header=elf64?64:52,entry=elf64?56:32;
    const auto offset=auth_field(mdt,elf64?32:28,elf64?8:4);
    const auto count=auth_field(mdt,elf64?56:44,2);
    need(auth_field(mdt,elf64?52:40,2)==header&&auth_field(mdt,elf64?54:42,2)==entry&&count==9&&
         offset>=header&&offset<=mdt.size()&&count*entry<=mdt.size()-offset,
         "Authentication program table differs from the retained nine-part image");
    Bytes output(auth_bytes),written(auth_bytes);
    for(size_t i=0;i<count;i++) {
        const size_t p=offset+i*entry;
        const auto at=auth_field(mdt,p+(elf64?8:4),elf64?8:4);
        const auto bytes=auth_field(mdt,p+(elf64?32:16),elf64?8:4);
        need(at<=auth_bytes&&bytes<=auth_bytes-at,"Authentication segment exceeds the fixed image");
        if(!bytes)continue;
        char name[16];snprintf(name,sizeof(name),"DEVAUTH.B%02u",unsigned(i));
        auto part=read(std::string(name),auth_bytes);
        need(part.size()==bytes,"Authentication split size differs from its metadata");
        for(size_t j=0;j<bytes;j++) {
            need(!written[at+j]||output[at+j]==part[j],"Authentication split overlaps disagree");
            output[at+j]=part[j];written[at+j]=1;
        }
    }
    return output;
}
static inline bool admitted_auth(const Bytes &bytes) {return bytes.size()==auth_bytes&&digest(bytes)==auth_sha;}
static inline Bytes auth_image(const Fat &fat) {
    const auto mdt=fat.image_file("DEVAUTH.MDT",1024*1024);
    auto bytes=join_auth_parts(mdt,[&](const auto &name,size_t maximum){return fat.image_file(name,maximum);});
    need(admitted_auth(bytes),"Reconstructed authentication image is not the retained Uke TA");
    return bytes;
}
} // namespace uke_local_fw
