// SPDX-License-Identifier: MIT
// All fault injection stays in this host-only executable.
#include "layout-fixture.hpp"
#include <iostream>
#include <linux/blkpg.h>
#include <map>
#include <sys/ioctl.h>
static int fault_fd=-1;
static unsigned writes=0,syncs=0,reads=0,fail_write=0,fail_sync=0,fail_read=0;
static bool torn=false,short_pending=false,backup_synced=false;
static std::uint64_t primary_table=0;
static ssize_t fault_pwrite(int fd,const void* data,size_t count,off_t at) {
    if(fd==fault_fd) {
        ++writes;
        if(at>=static_cast<off_t>(primary_table)&&at<static_cast<off_t>(primary_table+16384))assert(backup_synced);
        if(at==4096)assert(backup_synced);
        if(short_pending){short_pending=false;errno=EIO;return -1;}
        if(writes==fail_write){if(torn){short_pending=true;return ::pwrite(fd,data,count/2,at);}errno=EIO;return -1;}
    }
    return ::pwrite(fd,data,count,at);
}
static int fault_fsync(int fd) {
    if(fd==fault_fd) {
        if(++syncs==fail_sync){errno=EIO;return -1;}
        const auto table=ure::gpt_inspect(fd,4096);
        if(syncs>=2&&table["backup"]["valid"]==true)backup_synced=true;
    }return ::fsync(fd);
}
static int fault_advise(int fd,off_t at,off_t length,int advice) {
    if(fd==fault_fd&&++reads==fail_read)return EIO;
    return ::posix_fadvise(fd,at,length,advice);
}
static constexpr int kernel_fd=9876;
static std::map<int,std::pair<long long,long long>> kernel_rows;
static unsigned kernel_calls=0,kernel_fail=0,undo_fail=0;
static int fault_ioctl(int fd,unsigned long request,void* data=nullptr) {
    if(fd!=kernel_fd)return ::ioctl(fd,request,data);
    assert(request==BLKPG);++kernel_calls;
    if(kernel_calls==kernel_fail||kernel_calls==undo_fail){errno=EBUSY;return -1;}
    const auto& args=*static_cast<blkpg_ioctl_arg*>(data);const auto& row=*static_cast<blkpg_partition*>(args.data);
    if(args.op==BLKPG_DEL_PARTITION){if(!kernel_rows.erase(row.pno)){errno=ENOENT;return -1;}return 0;}
    if(args.op==BLKPG_ADD_PARTITION&&kernel_rows.contains(row.pno)){errno=EEXIST;return -1;}
    if(args.op==BLKPG_RESIZE_PARTITION&&!kernel_rows.contains(row.pno)){errno=ENOENT;return -1;}
    for(const auto& [index,range]:kernel_rows)if(index!=row.pno)assert(row.start+row.length<=range.first||range.first+range.second<=row.start);
    kernel_rows[row.pno]={row.start,row.length};return 0;
}
#define pwrite fault_pwrite
#define fsync fault_fsync
#define posix_fadvise fault_advise
#define ioctl fault_ioctl
#include "../src/layout-execute.cpp"
#undef pwrite
#undef fsync
#undef posix_fadvise
#undef ioctl
int main() {
    using namespace fixture;namespace fs=std::filesystem;
    char pattern[]="/tmp/uke-layout-safety-XXXXXX";const auto made=mkdtemp(pattern);assert(made);const fs::path root=made;
    try {
        const auto file=root/"disk";image(file);auto target=ure::storage_image(file,4096);ure::Root system("/");backup(target,root/"original","fixture");
        auto r=request();add(r,"esp","512","MiB","fat32");add(r,"linux","40","GiB","ext4");Evidence ev;auto p=propose(system,target,r,root/"original","fixture",{},ev,false,true);
        ure::Fd disk(open(file.c_str(),O_RDWR|O_CLOEXEC));fault_fd=disk.get();
        for(const auto& range:p.before)if(range.name=="primary_table")primary_table=range.offset;
        auto reset=[&]{fault_fd=-1;for(const auto& range:p.before)fixture::write(disk.get(),range.bytes,range.offset);assert(!::fsync(disk.get()));fault_fd=disk.get();writes=syncs=reads=fail_write=fail_sync=fail_read=0;torn=short_pending=backup_synced=false;};
        auto intact=[&]{const auto table=ure::gpt_inspect(disk.get(),4096);assert(table["primary"]["valid"]==true||table["backup"]["valid"]==true);assert(ure::storage_read(disk.get(),mib,27)=="OEM PAYLOAD MUST NOT CHANGE");
            for(const auto& before:p.before)if(before.name=="protective_mbr")assert(ure::storage_read(disk.get(),before.offset,before.bytes.size())==before.bytes);
        };
        reset();uke::layout::commit(disk.get(),p.before,p.after);const auto nwrites=writes,nsyncs=syncs,nreads=reads;assert(nwrites&&nsyncs&&nreads);intact();
        unsigned cases=0;
        for(unsigned i=1;i<=nwrites;++i)for(bool partial:{false,true}){reset();fail_write=i;torn=partial;refuse([&]{uke::layout::commit(disk.get(),p.before,p.after);});assert(writes==i+(partial?1:0));intact();++cases;}
        for(unsigned i=1;i<=nsyncs;++i){reset();fail_sync=i;refuse([&]{uke::layout::commit(disk.get(),p.before,p.after);});assert(syncs==i);intact();++cases;}
        for(unsigned i=1;i<=nreads;++i){reset();fail_read=i;refuse([&]{uke::layout::commit(disk.get(),p.before,p.after);});assert(reads==i);intact();++cases;}
        for(bool merging:{false,true}) {
            auto before=merging?p.document["new_rows"]:p.document["old_rows"],after=merging?p.document["old_rows"]:p.document["new_rows"];after[0]["role"]="userdata";
            auto changes=uke::layout::kernel_changes(before,after);
            for(unsigned failure=1;failure<=changes.size();++failure) {
                kernel_rows.clear();for(const auto& row:before)kernel_rows[row["index"].asInt()]={row["offset"].asUInt64(),row["bytes"].asUInt64()};const auto original=kernel_rows;
                kernel_calls=0;kernel_fail=failure;undo_fail=0;size_t applied=0;refuse([&]{uke::layout::kernel_layout(kernel_fd,changes,applied);});
                uke::layout::undo_kernel_layout(kernel_fd,changes,applied);assert(!applied&&kernel_rows==original);++cases;
            }
            kernel_rows.clear();for(const auto& row:before)kernel_rows[row["index"].asInt()]={row["offset"].asUInt64(),row["bytes"].asUInt64()};
            kernel_calls=0;kernel_fail=changes.size();undo_fail=changes.size()+1;size_t applied=0;refuse([&]{uke::layout::kernel_layout(kernel_fd,changes,applied);});
            const auto pending=applied;refuse([&]{uke::layout::undo_kernel_layout(kernel_fd,changes,applied);});assert(applied==pending&&kernel_calls==undo_fail);++cases;
        }
        fault_fd=-1;
        std::cout<<"Layout safety: "<<cases<<" GPT write/torn-write/barrier/readback and exact kernel undo failures passed; no blind retry, surviving GPT copy and OEM marker retained\n";
        fs::remove_all(root);
    }catch(...){fs::remove_all(root);throw;}
}
