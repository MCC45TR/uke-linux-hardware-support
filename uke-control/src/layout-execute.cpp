// SPDX-License-Identifier: MIT
#include "layout.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <linux/blkpg.h>
#include <linux/btrfs.h>
#include <linux/f2fs.h>
#include <linux/fs.h>
#include <map>
#include <memory>
#include <signal.h>
#include <sstream>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/sysmacros.h>
#include <unistd.h>
namespace uke::layout {
namespace fs=std::filesystem;
namespace {
void write_at(int fd,const std::string& data,std::uint64_t at) {
    size_t done=0;while(done<data.size()){const auto n=pwrite(fd,data.data()+done,data.size()-done,static_cast<off_t>(at+done));if(n<0&&errno==EINTR)continue;need(n>0,"GPT metadata write failed; retain the external journal");done+=size_t(n);}
}
void verify(int fd,const std::vector<StorageRange>& ranges) {
    for(const auto& r:ranges){need(!posix_fadvise(fd,r.offset,r.bytes.size(),POSIX_FADV_DONTNEED),"Cannot refresh GPT readback");need(ure::storage_read(fd,r.offset,r.bytes.size())==r.bytes,"GPT readback differs; retain the external journal");}
}
void commit(int fd,const std::vector<StorageRange>& before,const std::vector<StorageRange>& after) {
    verify(fd,before);
    try {
        for(const auto* name:{"backup_table","backup_header","primary_table","primary_header"}) {
            const auto& next=*std::find_if(after.begin(),after.end(),[&](const auto& r){return r.name==name;});
            const auto& old=*std::find_if(before.begin(),before.end(),[&](const auto& r){return r.name==name;});
            // Write only changed sectors; the protective MBR is never opened as a write range.
            for(size_t i=0;i<next.bytes.size();i+=4096)if(next.bytes.substr(i,4096)!=old.bytes.substr(i,4096))write_at(fd,next.bytes.substr(i,4096),next.offset+i);
            need(!fsync(fd)&&!posix_fadvise(fd,next.offset,next.bytes.size(),POSIX_FADV_DONTNEED),"Cannot synchronize GPT metadata");need(ure::storage_read(fd,next.offset,next.bytes.size())==next.bytes,"GPT metadata readback failed");
        }
        verify(fd,after);
    } catch(...) {
        // No format/growth has run yet; the preserved filesystem still fits the old partition.
        for(const auto* name:{"backup_table","backup_header","primary_table","primary_header"}) {
            const auto& r=*std::find_if(before.begin(),before.end(),[&](const auto& item){return item.name==name;});write_at(fd,r.bytes,r.offset);need(!fsync(fd),"GPT restore also failed; external recovery is required");
        }
        verify(fd,before);throw;
    }
}
std::string node(const StorageTarget& disk,const Value& row) {
    const auto index=row["index"].asUInt();const auto diskname=disk.identity["kernel_name"].asString();
    const auto name=diskname+(isdigit(static_cast<unsigned char>(diskname.back()))?"p":"")+std::to_string(index);
    const auto sys=fs::path("/sys/class/block")/name;
    need(fs::canonical(sys).parent_path()==fs::canonical("/"+disk.identity["sysfs_path"].asString()),"Partition no longer belongs to the admitted UFS LUN");
    need(std::stoull(read(sys/"start",64))*512==row["offset"].asUInt64()&&std::stoull(read(sys/"size",64))*512==row["bytes"].asUInt64(),"Kernel partition range differs from the reviewed plan");
    std::istringstream number(read(sys/"dev",64));unsigned a=0,b=0;char colon=0;number>>a>>colon>>b;need(number&&colon==':',"Invalid partition device number");
    const fs::path directory="/run/uke-control-layout-nodes";ure::private_directory(directory,!fs::exists(directory));
    const auto path=directory/name;struct stat st{};
    if(lstat(path.c_str(),&st)<0){need(errno==ENOENT&&!mknod(path.c_str(),S_IFBLK|0600,makedev(a,b)),"Cannot create a private partition handle");need(!lstat(path.c_str(),&st),"Cannot inspect private partition handle");}
    need(S_ISBLK(st.st_mode)&&st.st_uid==0&&(st.st_mode&0777)==0600&&st.st_nlink==1&&st.st_rdev==makedev(a,b),"Untrusted private partition handle");return path;
}
struct View {
    std::string name,path;bool active=false;
    View(const std::string& source,unsigned index) {
        ure::Fd check(open(source.c_str(),O_RDONLY|O_EXCL|O_CLOEXEC|O_NOFOLLOW));need(check.get()>=0,"Partition is mounted, swapped, held or unavailable");
        struct stat st{};need(!fstat(check.get(),&st)&&S_ISBLK(st.st_mode),"A real bounded partition is required");const auto bytes=ure::storage_bytes(check.get());check=ure::Fd();
        name="uke-layout-"+std::to_string(getpid())+"-"+std::to_string(index);
        checked({native_tool("dmsetup"),"--noudevsync","create",name,"--uuid","UKE-LAYOUT-"+name,"--table","0 "+std::to_string(bytes/512)+" linear "+std::to_string(major(st.st_rdev))+":"+std::to_string(minor(st.st_rdev))+" 0"});active=true;
        try {
            const auto r=run({native_tool("dmsetup"),"info","-c","--noheadings","--separator",":","-o","major,minor",name});need(r.status==0,"Cannot inspect bounded view");unsigned a=0,b=0;char colon=0;std::istringstream in(r.output);in>>a>>colon>>b;need(in&&colon==':',"Invalid bounded view identity");
            path="/run/uke-control-layout-nodes/"+name;need(!mknod(path.c_str(),S_IFBLK|0600,makedev(a,b)),"Cannot expose private bounded view");
            ure::Fd fd(open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW));need(fd.get()>=0&&ure::storage_bytes(fd.get())==bytes,"Bounded view size differs");
        }catch(...){close();throw;}
    }
    void close(){if(active){const auto r=run({native_tool("dmsetup"),"--noudevsync","remove",name});need(r.status==0,"Cannot release the owned bounded view: "+name);active=false;if(!path.empty())need(!unlink(path.c_str()),"Cannot remove the private bounded handle");}}
    ~View(){try{close();}catch(...){}}
    View(const View&)=delete;View& operator=(const View&)=delete;
};
void fscheck(const std::string& type,const std::string& path) {
    std::vector<std::string> args;
    if(type=="ext4")args={native_tool("e2fsck"),"-f","-n",path};
    else if(type=="f2fs")args={native_tool("fsck.f2fs"),"-f","--dry-run",path};
    else if(type=="btrfs")args={native_tool("btrfs"),"check","--readonly",path};
    else if(type=="xfs")args={native_tool("xfs_repair"),"-n",path};
    else if(type=="fat32")args={native_tool("fsck.fat"),"-n",path};
    else if(type=="ntfs")args={native_tool("ntfsfix"),"-n",path};
    else if(type=="exfat")args={native_tool("fsck.exfat"),"-n",path};
    else if(type=="linux-swap")return;
    else throw std::runtime_error("No independent filesystem checker");
    checked(args);
}
void format(const Value& row,const std::string& path) {
    const auto type=row["filesystem"].asString(),label=row["label"].asString();std::vector<std::string> args;
    if(type=="ext4")args={native_tool("mkfs.ext4"),"-F","-E","nodiscard","-L",label,path};
    else if(type=="f2fs")args={native_tool("mkfs.f2fs"),"-f","-t","0","-l",row["role"]=="userdata"?"UKE_LINUX":label,path};
    else if(type=="btrfs")args={native_tool("mkfs.btrfs"),"-f","-K","-L",label,path};
    else if(type=="xfs")args={native_tool("mkfs.xfs"),"-f","-K","-L",label.substr(0,12),path};
    else if(type=="fat32")args={native_tool("mkfs.vfat"),"-F","32","-n",label.substr(0,11),path};
    else if(type=="ntfs")args={native_tool("mkntfs"),"-F","-Q","-L",label,path};
    else if(type=="exfat")args={native_tool("mkfs.exfat"),"-K","-P","none","-L",label.substr(0,15),path};
    else if(type=="linux-swap")args={native_tool("mkswap"),"-L",label.substr(0,15),path};
    else throw std::runtime_error("No admitted formatter");
    // Erase signatures only inside the kernel-claimed partition view.
    checked({native_tool("wipefs"),"--all","--force",path});checked(args);fscheck(type,path);
    ure::Fd fd(open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC));const auto probe=filesystem_probe(fd.get(),0,ure::storage_bytes(fd.get()));const auto expected=type=="fat32"?"vfat":type=="linux-swap"?"swap":type;
    need(probe["type"]==expected&&!fsync(fd.get()),"Created filesystem signature or synchronization failed");
}
std::uint64_t le(const std::string& s){std::uint64_t out=0;for(size_t i=0;i<s.size();++i)out|=std::uint64_t(static_cast<unsigned char>(s[i]))<<(8*i);return out;}
void mounted_resize(const std::string& type,const std::string& mount,const std::string& device,std::uint64_t bytes) {
    ure::Fd fd(open(mount.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));struct stat st{},block{};
    need(fd.get()>=0&&!fstat(fd.get(),&st)&&!stat(device.c_str(),&block),"Cannot inspect resize mount or backing partition");
    std::uint64_t devid=0;
    need(type=="btrfs"?btrfs_mount(fd.get(),device,&devid):st.st_dev==block.st_rdev,"Resize mount no longer refers to the admitted partition");
    need(!syncfs(fd.get()),"Cannot synchronize filesystem before resize");
    if(type=="btrfs") {
        btrfs_ioctl_fs_info_args info{};need(!ioctl(fd.get(),BTRFS_IOC_FS_INFO,&info)&&info.num_devices==1,"Btrfs device set changed");
        btrfs_ioctl_vol_args args{};const auto text=std::to_string(devid)+":"+std::to_string(bytes);need(text.size()<sizeof(args.name),"Invalid Btrfs resize size");memcpy(args.name,text.c_str(),text.size()+1);need(!ioctl(fd.get(),BTRFS_IOC_RESIZE,&args),"Live Btrfs resize failed; no erase fallback was attempted");
    } else if(type=="f2fs") {
        ure::Fd disk(open(device.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC));const auto section=(1ULL<<le(ure::storage_read(disk.get(),1024+20,4)))*le(ure::storage_read(disk.get(),1024+24,4));
        need(section>0&&section<=1024*1024*1024,"Invalid F2FS section geometry");std::uint64_t blocks=(bytes/4096)/section*section;need(blocks>0&&!ioctl(fd.get(),F2FS_IOC_RESIZE_FS,&blocks),"Live F2FS shrink failed; no erase fallback was attempted");
    } else if(type=="ext4") {
        ure::Fd disk(open(device.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC));const auto shift=le(ure::storage_read(disk.get(),1024+24,4));need(shift<=6,"Invalid Ext4 block size");std::uint64_t blocks=bytes/(1024ULL<<shift);need(!ioctl(fd.get(),_IOW('f',16,__u64),&blocks),"Live Ext4 growth failed");
    } else if(type=="xfs") {
        // xfs_growfs operates on the held mount descriptor, not a user-supplied path.
        const auto r=run({native_tool("xfs_growfs"),"-d","/proc/self/fd/"+std::to_string(fd.get())},{},fd.get());need(r.status==0,"Live XFS growth failed: "+r.output);
    }else throw std::runtime_error("Unsupported live filesystem resize");
    need(!syncfs(fd.get()),"Cannot synchronize resized filesystem");
}
void resize(const Evidence& ev,const std::string& path,std::uint64_t bytes,bool shrink) {
    if(!ev.userdata_mount.empty()){mounted_resize(ev.filesystem,ev.userdata_mount,path,bytes);return;}
    View view(path,ev.userdata_index);fscheck(ev.filesystem,view.path);
    if(ev.filesystem=="ext4")checked({native_tool("resize2fs"),view.path,std::to_string(bytes/1024)+"K"});
    else if(ev.filesystem=="f2fs"&&!shrink)checked({native_tool("resize.f2fs"),view.path});
    else {
        const fs::path mountpoint="/run/uke-control-layout-mount";ure::private_directory(mountpoint,!fs::exists(mountpoint));
        need(!::mount(view.path.c_str(),mountpoint.c_str(),ev.filesystem.c_str(),MS_NODEV|MS_NOSUID|MS_NOEXEC,"nodiscard"),"Cannot mount isolated filesystem for native resize");
        try{mounted_resize(ev.filesystem,mountpoint,view.path,bytes);}catch(...){umount(mountpoint.c_str());throw;}
        need(!umount(mountpoint.c_str()),"Cannot unmount isolated resized filesystem");
    }
    fscheck(ev.filesystem,view.path);view.close();
}
void blkpg(int fd,int op,const Value& row) {
    blkpg_partition part{};part.pno=row["index"].asInt();part.start=static_cast<long long>(row["offset"].asUInt64());part.length=static_cast<long long>(row["bytes"].asUInt64());
    blkpg_ioctl_arg args{};args.op=op;args.datalen=sizeof(part);args.data=&part;need(!ioctl(fd,BLKPG,&args),"Kernel partition update was refused; no GPT commit is permitted");
}
void kernel_layout(int fd,const Value& old,const Value& next) {
    for(const auto& row:old)if(row["label"]!="userdata")blkpg(fd,BLKPG_DEL_PARTITION,row);
    for(const auto& row:next)blkpg(fd,row["role"]=="userdata"?BLKPG_RESIZE_PARTITION:BLKPG_ADD_PARTITION,row);
}
}
void execute(Inspection& current,const Proposal& proposal,const fs::path& job,bool shutdown,const Progress& progress) {
    const auto& plan=proposal.document;const auto& ev=current.evidence;
    need(plan["route"]!="blocked"&&plan["no_changes_required"]!=true,"There is no admitted layout operation");
    need(shutdown||plan["route"]=="direct","This operation requires the shutdown RAM worker");
    for(const auto& row:plan["old_rows"]) {
        const auto index=row["index"].asUInt();
        if(shutdown)need(!ev.mounted.contains(index),"Affected partitions must be unmounted after oldroot shutdown");
        need(!ev.holders.contains(index)&&!ev.swaps.contains(index),"Affected swap or device holders require shutdown; they are never disabled automatically");
    }
    power();need(plan["finish"]!="fastboot"||fastboot_available(),"Bound nvmem bootloader restart support is required");
    native_tool("dmsetup");native_tool("wipefs");
    if(plan["preserve_userdata"]==true) {
        if(ev.filesystem=="ext4"){native_tool("e2fsck");native_tool("resize2fs");}
        if(ev.filesystem=="f2fs"){native_tool("fsck.f2fs");native_tool("resize.f2fs");}
        if(ev.filesystem=="btrfs")native_tool("btrfs");
        if(ev.filesystem=="xfs"){native_tool("xfs_repair");native_tool("xfs_growfs");}
    }
    for(const auto& row:plan["old_rows"])if(row["label"]!="userdata"||!plan["preserve_userdata"].asBool()) {
        const auto path=node(current.target,row);ure::Fd claim(open(path.c_str(),O_RDONLY|O_CLOEXEC|O_EXCL|O_NOFOLLOW));need(claim.get()>=0,"A partition is still in use");
    }
    const auto diskpath="/dev/"+current.target.identity["kernel_name"].asString();ure::Fd disk(open(diskpath.c_str(),O_RDWR|O_NOFOLLOW|O_CLOEXEC|(shutdown?O_EXCL:0)));struct stat before{},now{};
    need(disk.get()>=0&&!fstat(disk.get(),&now)&&!fstat(current.target.descriptor.get(),&before)&&now.st_rdev==before.st_rdev&&S_ISBLK(now.st_mode),"Cannot acquire the exact admitted disk");
    // Holding O_EXCL on the disk prevents partition views; per-partition DM claims protect filesystem operations.
    if(shutdown){disk=ure::Fd(open(diskpath.c_str(),O_RDWR|O_NOFOLLOW|O_CLOEXEC));need(disk.get()>=0,"Cannot retain writable disk handle");}
    Value state;state["schema"]="org.uke.control.layout.state.v1";state["plan_sha256"]=plan["sha256"];state["completed"]=false;
    auto phase=[&](const std::string& text){state["phase"]=text;state["updated_utc"]=ure::utc();save_private(job/"state.json",state,fs::exists(job/"state.json"));if(progress)progress(text);};
    bool kernel_changed=false,gpt_committed=false;
    try {
        verify(disk.get(),proposal.before);phase("checking-filesystems");
        const bool preserve=plan["preserve_userdata"].asBool();const auto targetbytes=plan["new_rows"][0]["bytes"].asUInt64();const auto userpath=node(current.target,plan["old_rows"][0]);
        if(preserve&&ev.userdata_mount.empty()){View view(userpath,ev.userdata_index);fscheck(ev.filesystem,view.path);view.close();}
        if(preserve&&ev.filesystem_bytes>targetbytes){phase("shrinking-userdata");resize(ev,userpath,targetbytes,true);}
        if(preserve) {
            auto measured=filesystem_probe(disk.get(),plan["pool"]["offset"].asUInt64(),ev.partition_bytes);
            need(measured["size_verified"]==true&&measured["filesystem_bytes"].asUInt64()<=targetbytes,"Filesystem still exceeds its planned partition; GPT remains unchanged");
        }
        phase("updating-kernel-partitions");
        if(!shutdown) {
            try{kernel_changed=true;kernel_layout(disk.get(),plan["old_rows"],plan["new_rows"]);}
            catch(...){std::ofstream hint("/run/uke-control-layout-require-shutdown");hint<<"Kernel refused a live partition update\n";throw;}
        }
        std::map<unsigned,std::unique_ptr<View>> claims;
        if(!shutdown)for(const auto& row:plan["new_rows"])if(row["role"]!="userdata"||!preserve)
            claims.emplace(row["index"].asUInt(),std::make_unique<View>(node(current.target,row),row["index"].asUInt()));
        phase("committing-gpt");commit(disk.get(),proposal.before,proposal.after);gpt_committed=true;
        if(shutdown)need(!ioctl(disk.get(),BLKRRPART),"GPT committed but kernel reread failed; stop and recover using the journal");
        else {const auto event=fs::path("/sys/class/block")/current.target.identity["kernel_name"].asString()/"uevent";std::ofstream out(event);out<<"change\n";}
        phase("initializing-filesystems");
        for(const auto& row:plan["new_rows"]) {
            const auto path=node(current.target,row);
            if(row["role"]=="userdata"&&preserve) {
                auto after=ev;after.partition_bytes=row["bytes"].asUInt64();
                const auto measured=filesystem_probe(disk.get(),row["offset"].asUInt64(),row["bytes"].asUInt64());
                if(measured["filesystem_bytes"].asUInt64()+16*1048576<row["bytes"].asUInt64())resize(after,path,row["bytes"].asUInt64(),false);
            }else {
                const auto index=row["index"].asUInt();
                if(!claims.contains(index))claims.emplace(index,std::make_unique<View>(path,index));
                format(row,claims.at(index)->path);claims.at(index)->close();
            }
        }
        phase("verifying-layout");verify(disk.get(),proposal.after);need(ure::gpt_inspect(disk.get(),4096)["healthy"]==true,"Final GPT copies do not agree");
        if(plan["preserve_userdata"]==true) {
            const auto& row=plan["new_rows"][0];const auto fs=filesystem_probe(disk.get(),row["offset"].asUInt64(),row["bytes"].asUInt64());
            need(fs["size_verified"]==true&&fs["uuid"]==plan["userdata_uuid"]&&fs["filesystem_bytes"].asUInt64()<=row["bytes"].asUInt64(),"Final userdata geometry or preserved UUID failed verification");
        }
        Value receipt;receipt["schema"]="org.uke.control.layout.ownership.v1";receipt["completed"]=true;
        for(const auto* key:{"disk_guid","unit_identity_sha256","lun_address"})receipt[key]=current.target.identity[key];
        receipt["original_manifest_sha256"]=plan["original_manifest_sha256"];receipt["owned_rows"]=Value(Json::arrayValue);
        for(const auto& row:plan["new_rows"])if(row["role"]!="userdata")receipt["owned_rows"].append(row);
        receipt["plan_sha256"]=plan["sha256"];receipt["sha256"]=seal(receipt);save_private(job/"ownership.json",receipt);
        state["completed"]=true;phase("completed");
    }catch(const std::exception& error) {
        if(kernel_changed&&!gpt_committed){try{for(const auto& row:plan["new_rows"])if(row["role"]!="userdata") {try{blkpg(disk.get(),BLKPG_DEL_PARTITION,row);}catch(...){}}for(const auto& row:plan["old_rows"])blkpg(disk.get(),row["label"]=="userdata"?BLKPG_RESIZE_PARTITION:BLKPG_ADD_PARTITION,row);}catch(...){state["kernel_restore_failed"]=true;}}
        state["error"]=error.what();state["gpt_committed"]=gpt_committed;phase("failed-no-automatic-retry");throw;
    }
}
Value run_worker(const fs::path& request,bool shutdown,const Progress& progress) {
    ure::Fd lock(open("/run/uke-control-layout.lock",O_RDWR|O_CREAT|O_NOFOLLOW|O_CLOEXEC,0600));need(lock.get()>=0&&!flock(lock.get(),LOCK_EX|LOCK_NB),"Another layout operation is active");
    const auto job=fs::canonical(request.parent_path());auto plan=load_private(job/"plan.json");auto consent=load_private(request);
    need(plan["schema"]=="org.uke.control.layout.plan.v1"&&plan["sha256"]==seal(plan)&&plan["engine_version"]==engine_version&&plan["recovery_source"]==recovery_source,"Layout plan schema, source or digest differs");
    need(consent["plan_sha256"]==plan["sha256"]&&consent["sha256"]==seal(consent)&&consent["approvals"]==plan["confirmations_required"]&&consent["boot_id_sha256"]==ure::sha256(read("/proc/sys/kernel/random/boot_id"))&&consent["expires_ms"].asUInt64()>=ure::monotonic_ms(),"Layout consent is missing, stale, incomplete or belongs to another boot");
    need(!fs::exists(job/"state.json"),"This job already started; automatic retries and stale approvals are refused");
    auto current=inspect();ure::Root system("/");Value receipt;if(fs::exists(job/"previous-ownership.json"))receipt=load_private(job/"previous-ownership.json");
    auto fresh=propose(system,current.target,plan["request"],job/"original-gpt",plan["firmware_profile"].asString(),receipt,current.evidence,plan["preserve_userdata"].asBool(),plan["finish"]=="fastboot");
    for(const auto* key:{"request","original_manifest_sha256","pool","before","after","old_rows","new_rows","protected_records","userdata_filesystem","userdata_uuid"})need(ure::json(fresh.document[key])==ure::json(plan[key]),std::string("Layout changed after confirmation: ")+key);
    for(const auto* key:{"unit_identity_sha256","lun_address","disk_guid","bytes","logical_sector_bytes","boot_id_sha256"})need(ure::json(current.target.identity[key])==ure::json(plan["target_identity"][key]),"Target identity changed after confirmation");
    auto locator=journal_identity(job,current.target);need(locator["uuid"]==consent["journal"]["uuid"]&&locator["relative"]==consent["journal"]["relative"],"External journal identity changed");
    need(fresh.document["blockers"].empty(),"Fresh layout admission has blockers");
    if(!shutdown)need(fresh.document["route"]=="direct","Live execution is no longer admitted");
    // The reviewed route may become direct after oldroot is unmounted; stable geometry is unchanged.
    fresh.document=plan;
    sigset_t critical,previous;sigemptyset(&critical);for(int signal:{SIGINT,SIGTERM,SIGHUP})sigaddset(&critical,signal);need(!sigprocmask(SIG_BLOCK,&critical,&previous),"Cannot protect critical storage operation");
    try{execute(current,fresh,job,shutdown,progress);}catch(...){sigprocmask(SIG_SETMASK,&previous,nullptr);throw;}
    sigprocmask(SIG_SETMASK,&previous,nullptr);return load_private(job/"state.json");
}
}
