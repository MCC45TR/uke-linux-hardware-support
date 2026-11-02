// SPDX-License-Identifier: MIT
#define UKE_KERNEL_UPDATE_TEST
#include "../src/kernel-update.cpp"
#include <cassert>
#include <functional>
#include <iterator>
static void reject(const std::function<void()> &fn) {bool failed=false;try{fn();}catch(const std::exception&){failed=true;}assert(failed);}
static void put32(Bytes &b,size_t p,uint32_t v){for(unsigned i=0;i<4;++i)b.at(p+i)=v>>(8*i);}
static void put64(Bytes &b,size_t p,uint64_t v){put32(b,p,v);put32(b,p+4,v>>32);}
static void sign(Bytes &b,size_t checksum,size_t begin,size_t size) {
    std::fill(b.begin()+checksum,b.begin()+checksum+32,0);
    const auto hash=digest(Bytes(b.begin()+begin,b.begin()+begin+size));
    for(unsigned i=0;i<32;++i)b[checksum+i]=std::stoul(hash.substr(i*2,2),nullptr,16);
}
static constexpr size_t total=8*1024*1024,geometry=4096,header=12288,tables=header+256,backup=16384;
static Bytes layout() {
    Bytes b(total);put32(b,geometry,0x616c4467);put32(b,geometry+4,52);
    put32(b,geometry+40,4096);put32(b,geometry+44,1);put32(b,geometry+48,4096);
    put32(b,header,0x414c5030);put32(b,header+4,0x0002000a);put32(b,header+8,256);put32(b,header+44,188);
    for(const auto &[offset,index,count,width]:std::vector<std::array<uint32_t,4>>{{80,0,1,52},{92,52,1,24},{104,76,1,48},{116,124,1,64}}) {
        put32(b,header+offset,index);put32(b,header+offset+4,count);put32(b,header+offset+8,width);
    }
    memcpy(b.data()+tables,"odm_a",6);put32(b,tables+36,1);put32(b,tables+44,1);
    put64(b,tables+52,2048);put64(b,tables+64,128);
    memcpy(b.data()+tables+76,"default",8);
    put64(b,tables+124,64);put32(b,tables+132,4096);put64(b,tables+140,total);
    memcpy(b.data()+tables+148,"super",6);return b;
}
static void reseal(Bytes &b) {
    sign(b,geometry+8,geometry,52);std::copy_n(b.begin()+geometry,4096,b.begin()+8192);
    // The table checksum is outside the tables, so compute it without zeroing table bytes.
    const auto hash=digest(Bytes(b.begin()+tables,b.begin()+tables+188));
    for(unsigned i=0;i<32;++i)b[header+48+i]=std::stoul(hash.substr(i*2,2),nullptr,16);
    sign(b,header+12,header,256);std::copy_n(b.begin()+header,4096,b.begin()+backup);
}
static void write_all(int fd,const Bytes &b) {
    size_t done=0;while(done<b.size()){const auto n=pwrite(fd,b.data()+done,b.size()-done,done);assert(n>0);done+=size_t(n);}assert(!fsync(fd));
}
int main(int argc,char **argv) {
    if(argc==2 && std::string(argv[1])=="--close-output-wait") {close(STDOUT_FILENO);sleep(30);return 0;}
    assert(argc==2);
    char temporary[]="/tmp/uke-oem-video.XXXXXX";assert(mkdtemp(temporary));const fs::path root=temporary;
    auto base=layout();reseal(base);save_new(root/"super.img",base);
    Fd fd(open((root/"super.img").c_str(),O_RDWR|O_CLOEXEC));
    const auto good=uke_video::partitions(fd.value,total);
    assert(good.size()==1 && good.at("odm_a").offset==128*512 && good.at("odm_a").size==2048*512);
    const auto semantic=[&](const std::function<void(Bytes&)> &change) {auto b=layout();change(b);reseal(b);write_all(fd.value,b);reject([&]{uke_video::partitions(fd.value,total);});};
    semantic([](Bytes &b){put32(b,geometry,0);});
    semantic([](Bytes &b){put32(b,geometry+40,2*1024*1024);});
    semantic([](Bytes &b){put32(b,geometry+44,0);});
    semantic([](Bytes &b){put32(b,header+4,11);});
    semantic([](Bytes &b){put32(b,header+8,8192);});
    semantic([](Bytes &b){put32(b,header+128,2);});
    semantic([](Bytes &b){put32(b,header+88,53);});
    semantic([](Bytes &b){put32(b,header+92,0);});
    semantic([](Bytes &b){std::fill(b.begin()+tables,b.begin()+tables+36,'x');});
    semantic([](Bytes &b){put32(b,tables+36,0);});
    semantic([](Bytes &b){put32(b,tables+44,2);});
    semantic([](Bytes &b){put32(b,tables+48,1);});
    semantic([](Bytes &b){put32(b,tables+60,1);});
    semantic([](Bytes &b){put32(b,tables+72,1);});
    semantic([](Bytes &b){put64(b,tables+64,UINT64_MAX);});
    semantic([](Bytes &b){put64(b,tables+52,UINT64_MAX);});
    semantic([](Bytes &b){put64(b,tables+64,1);});
    semantic([](Bytes &b){put64(b,tables+140,total+512);});
    for(const auto p:{geometry+8,header+12,tables+2,backup+12,8192+4}) {
        auto b=base;b[p]^=1;write_all(fd.value,b);reject([&]{uke_video::partitions(fd.value,total);});
    }
    auto vendor=layout();memcpy(vendor.data()+tables,"vendor_a",9);reseal(vendor);write_all(fd.value,vendor);
    assert(uke_video::partitions(fd.value,total).contains("vendor_a"));
    write_all(fd.value,base);reject([&]{uke_video::partitions(fd.value,total-512);});
    const std::string expected("fixture\0payload",15);
    const auto extracted=uke_video::capture({"/usr/sbin/debugfs","-R","cat /firmware/vpu30_2v.mbn",argv[1]});
    assert(std::string(extracted.begin(),extracted.end())==expected);
    assert(uke_video::capture({"/usr/sbin/debugfs","-R","cat /missing",argv[1]}).empty());
    assert(!uke_video::admitted(extracted) && !uke_video::admitted(Bytes{}));
    reject([&]{uke_video::capture({"/usr/bin/false"});});
    reject([&]{uke_video::capture({"/usr/bin/head","-c","8192","/dev/zero"},1024);});
    const auto before=std::chrono::steady_clock::now();
    reject([&]{uke_video::capture({fs::absolute(argv[0]).string(),"--close-output-wait"});});
    const auto elapsed=std::chrono::steady_clock::now()-before;
    assert(elapsed>=std::chrono::seconds(14) && elapsed<std::chrono::seconds(20));
    const auto destination=root/"firmware/release/qcom/uke/vpu30_2v.mbn";
    uke_video::publish(destination,extracted);assert(file(destination)==extracted);
    const auto entries=std::distance(fs::directory_iterator(destination.parent_path()),fs::directory_iterator{});
    uke_video::publish(destination,extracted);
    reject([&]{uke_video::publish(destination,Bytes{'x'});});assert(file(destination)==extracted);
    assert(std::distance(fs::directory_iterator(destination.parent_path()),fs::directory_iterator{})==entries);
    const auto linked=destination.parent_path()/"linked";fs::create_symlink(destination,linked);
    reject([&]{uke_video::publish(linked,extracted);});
    fs::create_hard_link(destination,destination.parent_path()/"hard");
    reject([&]{uke_video::publish(destination,extracted);});
    fs::remove_all(root);
    std::cout<<"LP mirrored checksums/geometry/range/identity refusals, bounded real debugfs extraction, child deadline and atomic no-overwrite publication passed; regular files only\n";
}
