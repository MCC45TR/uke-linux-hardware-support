// SPDX-License-Identifier: MIT
#pragma once
#include "layout.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <fcntl.h>
#include <unistd.h>
namespace fixture {
using namespace uke::layout;
constexpr std::uint64_t mib=1048576,gib=1073741824,capacity=256*gib;
inline void put(std::string& bytes,size_t offset,std::uint64_t value,unsigned count){for(unsigned i=0;i<count;++i)bytes.at(offset+i)=char(value>>(8*i));}
inline std::uint32_t crc(std::string_view bytes){std::uint32_t value=UINT32_MAX;for(unsigned char c:bytes){value^=c;for(unsigned i=0;i<8;++i)value=(value>>1)^((value&1)?0xedb88320U:0);}return value^UINT32_MAX;}
inline void write(int fd,const std::string& bytes,std::uint64_t offset){size_t done=0;while(done<bytes.size()){auto n=pwrite(fd,bytes.data()+done,bytes.size()-done,offset+done);assert(n>0);done+=n;}}
inline void image(const std::filesystem::path& path) {
    ure::Fd fd(open(path.c_str(),O_RDWR|O_CREAT|O_EXCL|O_CLOEXEC,0600));assert(fd.get()>=0&&!ftruncate(fd.get(),capacity));constexpr unsigned sector=4096;const auto sectors=capacity/sector,last=sectors-6,backup=last+1;
    std::string table(16384,'\0');
    auto entry=[&](unsigned index,std::uint64_t first,std::uint64_t end,const std::string& label,bool reserved=false) {
        const auto at=index*128;for(unsigned i=0;i<16;++i){if(!reserved)table[at+i]=char(i+11);table[at+16+i]=char(i+31+index);}
        put(table,at+32,first,8);put(table,at+40,end,8);if(reserved)put(table,at+48,1ULL<<60,8);for(size_t i=0;i<label.size();++i)table[at+56+2*i]=label[i];
    };
    for(unsigned i=0;i<29;++i)entry(i,(i+1)*mib/sector,(i+2)*mib/sector-1,i==9?"countrycode_a":i==10?"countrycode_b":"oem_"+std::to_string(i));
    table[9*128]^=1;table[10*128]^=2;put(table,9*128+48,1ULL<<50,8);
    entry(29,64*mib/sector,250*gib/sector-1,"userdata");entry(30,32*mib/sector,33*mib/sector-1,"factory");entry(31,last,last,"last_parti",true);
    write(fd.get(),table,2*sector);write(fd.get(),table,backup*sector);
    for(const auto lba:std::array<std::uint64_t,2>{1,sectors-1}) {
        std::string h(sector,'\0');h.replace(0,8,"EFI PART");put(h,8,0x10000,4);put(h,12,92,4);put(h,24,lba,8);put(h,32,sectors-lba,8);put(h,40,6,8);put(h,48,last,8);
        for(unsigned i=0;i<16;++i)h[56+i]=char(i+81);
        put(h,72,lba==1?2:backup,8);put(h,80,32,4);put(h,84,128,4);put(h,88,crc(std::string_view(table).substr(0,32*128)),4);put(h,16,crc(std::string_view(h).substr(0,92)),4);write(fd.get(),h,lba*sector);
    }
    std::string mbr(sector,'\0');mbr[450]=char(0xee);put(mbr,454,1,4);put(mbr,458,UINT32_MAX,4);mbr[510]=0x55;mbr[511]=char(0xaa);write(fd.get(),mbr,0);write(fd.get(),"OEM PAYLOAD MUST NOT CHANGE",mib);assert(!fsync(fd.get()));
}
inline Value request(bool single=false) {
    Value out;out["schema"]=1;out["format"]="uke-multiboot-request";out["action"]=single?"restore-default":"setup";out["advanced"]=false;out["keep_userdata"]=true;out["userdata_filesystem"]="ext4";out["partitions"]=Value(Json::arrayValue);return out;
}
inline void add(Value& request,const char* role,const char* size,const char* unit,const char* fs){Value row;row["role"]=role;row["size"]=size;row["unit"]=unit;row["filesystem"]=fs;request["partitions"].append(row);}
inline Value receipt(const Value& p,const Value& identity) {
    Value out;out["schema"]="org.uke.control.layout.ownership.v1";out["completed"]=true;for(auto* k:{"disk_guid","unit_identity_sha256","lun_address"})out[k]=identity[k];out["original_manifest_sha256"]=p["original_manifest_sha256"];out["owned_rows"]=Value(Json::arrayValue);for(auto& row:p["new_rows"])if(row["role"]!="userdata")out["owned_rows"].append(row);out["sha256"]=seal(out);return out;
}
template<class F>inline void refuse(F&& fn){bool failed=false;try{fn();}catch(const std::exception&){failed=true;}assert(failed);}
}
