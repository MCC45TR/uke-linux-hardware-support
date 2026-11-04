// SPDX-License-Identifier: MIT
#pragma once
#include "local.hpp"
namespace uke_local_fw {
struct FatEntry {uint32_t cluster,size;bool directory;};
class Fat {
    int fd_;uint64_t limit_,fat_,mirror_,data_,root_;uint32_t clusters_,root_cluster_,root_bytes_,cluster_bytes_;bool fat32_;
    Bytes range(uint64_t offset,size_t size)const {
        need(offset<=limit_&&size<=limit_-offset,"FAT read exceeds its volume");return read_fd(fd_,size,offset);
    }
    static uint16_t u16(const Bytes &b,size_t p){need(p<=b.size()&&b.size()-p>=2,"Short FAT field");return b[p]|uint16_t(b[p+1])<<8;}
    static uint32_t u32(const Bytes &b,size_t p){return u16(b,p)|uint32_t(u16(b,p+2))<<16;}
    uint32_t next(uint32_t cluster)const {
        need(cluster>=2&&cluster<clusters_+2,"FAT cluster outside data area");
        const uint64_t pos=uint64_t(cluster)*(fat32_?4:2);auto a=range(fat_+pos,fat32_?4:2),b=range(mirror_+pos,fat32_?4:2);
        need(a==b,"FAT allocation mirrors disagree");const auto value=fat32_?u32(a,0)&0x0fffffff:u16(a,0);
        if(value>=(fat32_?0x0ffffff8U:0xfff8U))return 0;
        need(value>=2&&value<clusters_+2,"Free, reserved or invalid FAT link");return value;
    }
    uint64_t offset(uint32_t cluster)const {
        need(cluster>=2&&cluster<clusters_+2,"Invalid FAT cluster");return data_+uint64_t(cluster-2)*cluster_bytes_;
    }
    Bytes chain(uint32_t cluster,size_t maximum,bool exact)const {
        Bytes out;std::set<uint32_t> seen;
        while(cluster) {
            need(seen.insert(cluster).second&&seen.size()<=65536,"Cyclic or oversized FAT chain");
            auto bytes=range(offset(cluster),cluster_bytes_);
            const auto count=std::min(maximum-out.size(),bytes.size());out.insert(out.end(),bytes.begin(),bytes.begin()+count);
            const auto following=next(cluster);
            if(out.size()==maximum){need(!exact||!following,"FAT file owns excess clusters");return out;}
            cluster=following;
        }
        need(!exact||out.size()==maximum,"FAT file chain is truncated");return out;
    }
    static char upper(unsigned char c) {return c>='a'&&c<='z'?c-32:c;}
    static std::string short_name(const unsigned char *p) {
        std::string base(reinterpret_cast<const char *>(p),8),ext(reinterpret_cast<const char *>(p+8),3);
        while(!base.empty()&&base.back()==' ')base.pop_back();while(!ext.empty()&&ext.back()==' ')ext.pop_back();
        auto name=base+(ext.empty()?"":"."+ext);
        std::transform(name.begin(),name.end(),name.begin(),upper);return name;
    }
    struct LongName {
        std::array<uint16_t,260> text{};
        unsigned remaining=0,count=0;unsigned char checksum=0;
        bool active=false;
        void reset(){active=false;remaining=count=0;}
        void append(const Bytes &dir,size_t p) {
            const unsigned order=dir[p]&0x1f;
            need(!(dir[p]&0xa0)&&order&&order<=20&&!dir[p+12]&&!u16(dir,p+26),"Invalid FAT long-name slot");
            if(dir[p]&0x40) {
                need(!active,"Interrupted FAT long-name sequence");
                text.fill(0xffff);remaining=count=order;checksum=dir[p+13];active=true;
            }
            need(active&&remaining==order&&checksum==dir[p+13],"Unordered FAT long-name sequence");
            constexpr std::array<unsigned,13> offsets={1,3,5,7,9,14,16,18,20,22,24,28,30};
            for(size_t i=0;i<offsets.size();i++)text[(order-1)*13+i]=u16(dir,p+offsets[i]);
            --remaining;
        }
        std::string finish(const Bytes &dir,size_t p) {
            if(!active)return {};
            need(!remaining,"Truncated FAT long-name sequence");
            unsigned char actual=0;
            for(size_t i=0;i<11;i++)actual=static_cast<unsigned char>(((actual&1)?0x80:0)+(actual>>1)+dir[p+i]);
            need(actual==checksum,"FAT long-name checksum mismatch");
            std::string name;bool terminated=false,ascii=true;size_t length=0;
            for(size_t i=0;i<count*13;i++) {
                const auto c=text[i];
                if(terminated){need(c==0xffff,"Invalid FAT long-name padding");continue;}
                if(!c){terminated=true;continue;}
                need(c!=0xffff&&++length<=255,"Invalid FAT long-name length");
                if(c<0x20||c>0x7e)ascii=false;
                else name+=upper(static_cast<unsigned char>(c));
            }
            need(length,"Empty FAT long name");reset();
            return ascii?name:std::string{};
        }
    };
    FatEntry lookup(const Bytes &dir,const std::string &name)const {
        bool found=false;FatEntry entry{};LongName long_name;
        for(size_t p=0;p+32<=dir.size();p+=32) {
            if(!dir[p]){need(!long_name.active,"Dangling FAT long-name sequence");break;}
            if(dir[p]==0xe5){long_name.reset();continue;}
            if(dir[p+11]==0x0f){long_name.append(dir,p);continue;}
            const auto long_alias=long_name.finish(dir,p);
            if(dir[p+11]&8)continue;
            if(short_name(dir.data()+p)!=name&&long_alias!=name)continue;
            need(!found,"Duplicate FAT file entry");found=true;
            need(!(dir[p+11]&0xc0),"Unknown FAT attribute");
            entry={uint32_t(u16(dir,p+26))|(fat32_?uint32_t(u16(dir,p+20))<<16:0),u32(dir,p+28),bool(dir[p+11]&0x10)};
        }
        need(!long_name.active,"Dangling FAT long-name sequence");
        need(found,"Required fixed stock file is absent: "+name);return entry;
    }
public:
    Fat(int fd,uint64_t actual):fd_(fd),limit_(actual) {
        auto boot=range(0,512);need(boot[510]==0x55&&boot[511]==0xaa,"Invalid FAT boot signature");
        const uint32_t sector=u16(boot,11),spc=boot[13],reserved=u16(boot,14),copies=boot[16],entries=u16(boot,17);
        const uint32_t sectors=u16(boot,19)?u16(boot,19):u32(boot,32),fat_sectors=u16(boot,22)?u16(boot,22):u32(boot,36);
        need(sector>=512&&sector<=4096&&!(sector&(sector-1))&&spc&&spc<=128&&!(spc&(spc-1))&&
             reserved&&copies==2&&sectors&&fat_sectors,"Unsupported FAT geometry");
        const uint64_t root_sectors=(uint64_t(entries)*32+sector-1)/sector;
        const uint64_t first=reserved+uint64_t(copies)*fat_sectors+root_sectors;
        need(first<sectors&&uint64_t(sectors)*sector<=actual,"FAT volume exceeds the source partition");
        limit_=uint64_t(sectors)*sector;clusters_=(sectors-first)/spc;
        fat32_=!u16(boot,22)&&!entries;
        need(clusters_>=4085&&(fat32_?clusters_>=65525:clusters_<65525),"FAT12 or inconsistent FAT type refused");
        need(uint64_t(clusters_+2)*(fat32_?4:2)<=uint64_t(fat_sectors)*sector,"FAT allocation table is too short");
        if(fat32_)need(!(u16(boot,40)&0x80)&&!u16(boot,42),"Non-mirrored or versioned FAT32 refused");
        cluster_bytes_=sector*spc;fat_=uint64_t(reserved)*sector;mirror_=fat_+uint64_t(fat_sectors)*sector;
        root_=mirror_+uint64_t(fat_sectors)*sector;root_bytes_=root_sectors*sector;data_=first*sector;
        root_cluster_=fat32_?u32(boot,44):0;
    }
    Bytes image_file(const std::string &name,size_t maximum=24*1024*1024)const {
        need(name.size()<=12&&name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.")==std::string::npos,
             "Only fixed CDSP short filenames are admitted");
        auto root=fat32_?chain(root_cluster_,1024*1024,false):range(root_,root_bytes_);
        const auto image=lookup(root,"IMAGE");need(image.directory,"FAT IMAGE is not a directory");
        const auto dir=chain(image.cluster,1024*1024,false);
        const auto entry=lookup(dir,name);
        need(!entry.directory&&entry.size&&entry.size<=maximum,"FAT firmware file size is outside bounds");
        return chain(entry.cluster,entry.size,true);
    }
};
} // namespace uke_local_fw
