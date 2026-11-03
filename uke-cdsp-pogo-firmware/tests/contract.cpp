// SPDX-License-Identifier: MIT
#define UKE_LOCAL_FW_TEST
#define UKE_KERNEL_UPDATE_TEST
#include "../src/import.cpp"
#include <cassert>
using namespace uke_local_fw;
static void put16(Bytes &b,size_t p,uint16_t v){b.at(p)=v;b.at(p+1)=v>>8;}
static void put32(Bytes &b,size_t p,uint32_t v){put16(b,p,v);put16(b,p+2,v>>16);}
template<class F>static void refused(F f){bool no=false;try{f();}catch(const std::exception &){no=true;}assert(no);}
static Bytes fat16() {
    Bytes b(8192*512);put16(b,11,512);b[13]=1;put16(b,14,1);b[16]=2;put16(b,17,16);
    put16(b,19,8192);put16(b,22,32);b[510]=0x55;b[511]=0xaa;
    for(size_t base:{512U,33U*512}) {
        put16(b,base,0xfff8);put16(b,base+2,0xffff);put16(b,base+4,0xffff);put16(b,base+6,4);put16(b,base+8,0xffff);
    }
    const size_t root=65*512,data=66*512;
    memcpy(b.data()+root,"IMAGE      ",11);b[root+11]=0x10;put16(b,root+26,2);
    memcpy(b.data()+data,"CDSP    MDT",11);put16(b,data+26,3);put32(b,data+28,700);
    for(unsigned i=0;i<700;i++)b[data+512+i]=i;
    return b;
}
static void write_image(int fd,const Bytes &b){assert(!ftruncate(fd,0));size_t n=0;while(n<b.size()){const auto k=pwrite(fd,b.data()+n,b.size()-n,n);assert(k>0);n+=k;}}
static Bytes fat32() {
    Bytes b(70000*512);put16(b,11,512);b[13]=1;put16(b,14,32);b[16]=2;
    put32(b,32,70000);put32(b,36,600);put32(b,44,2);b[510]=0x55;b[511]=0xaa;
    for(size_t base:{32U*512,632U*512}) {
        put32(b,base,0x0ffffff8);put32(b,base+4,0x0fffffff);
        put32(b,base+8,0x0fffffff);put32(b,base+12,0x0fffffff);
        put32(b,base+16,5);put32(b,base+20,0x0fffffff);
    }
    const size_t data=1232*512;
    memcpy(b.data()+data,"IMAGE      ",11);b[data+11]=0x10;put16(b,data+26,3);
    memcpy(b.data()+data+512,"CDSP_DTBMDT",11);put16(b,data+512+26,4);put32(b,data+512+28,700);
    for(unsigned i=0;i<700;i++)b[data+1024+i]=i;
    return b;
}
int main() {
    char name[]="/tmp/uke-fw-test-XXXXXX";const auto root=mkdtemp(name);assert(root);
    const fs::path dir=root;Fd image(open((dir/"fat.img").c_str(),O_RDWR|O_CREAT|O_EXCL,0600));
    auto bytes=fat16();write_image(image.value,bytes);
    Fat fat(image.value,bytes.size());const auto payload=fat.image_file("CDSP.MDT");
    assert(payload.size()==700);for(unsigned i=0;i<payload.size();i++)assert(payload[i]==(i&255));
    refused([&]{fat.image_file("../CDSP.MDT");});
    refused([&]{fat.image_file("CDSP.MDT",699);});
    auto bad=bytes;put16(bad,33*512+6,5);write_image(image.value,bad);
    refused([&]{Fat f(image.value,bad.size());f.image_file("CDSP.MDT");});
    bad=bytes;for(size_t base:{512U,33U*512})put16(bad,base+8,3);write_image(image.value,bad);
    refused([&]{Fat f(image.value,bad.size());f.image_file("CDSP.MDT");});
    bad=bytes;put16(bad,19,65535);write_image(image.value,bad);refused([&]{Fat f(image.value,bad.size());});
    auto large=fat32();write_image(image.value,large);
    Fat f32(image.value,large.size());assert(f32.image_file("CDSP_DTB.MDT")==payload);
    large[40]=0x80;write_image(image.value,large);refused([&]{Fat f(image.value,large.size());});
    refused([&]{admit_mdt(payload,main_sha);});assert(!admit_mcu(Bytes(82740)));
    Bytes mdt(84);memcpy(mdt.data(),"\177ELF",4);mdt[4]=mdt[5]=mdt[6]=1;
    put32(mdt,28,52);put16(mdt,40,52);put16(mdt,42,32);put16(mdt,44,1);
    admit_mdt(mdt,digest(mdt).c_str());
    put32(mdt,28,84);refused([&]{admit_mdt(mdt,digest(mdt).c_str());});
    Family family{{"cdsp.mdt",Bytes{1,2,3}},{"cdsp.b00",Bytes{4,5}}};
    publish_family(dir/"out","cdsp",family);verify_family(dir/"out/cdsp",family);
    publish_family(dir/"out","cdsp",family);
    auto changed=family;changed["cdsp.b00"][0]=0;refused([&]{publish_family(dir/"out","cdsp",changed);});
    assert(file(dir/"out/cdsp/cdsp.b00")==family["cdsp.b00"]);
    fs::create_directory_symlink(dir/"out/cdsp",dir/"link");
    refused([&]{publish_family(dir/"link","nanosic",Family{{"MCU_Upgrade.bin",Bytes{1}}});});
    refused([&]{publish_family(dir/"out","nanosic",Family{{"../bad",Bytes{1}}});});
    need(!fs::exists(dir/"out/bad"),"Unsafe filename escaped stage");
    fs::remove_all(dir);
    std::cout<<"PASS: actual read-only FAT geometry/chains/mirrors, stock identity refusal and atomic no-replace publication; synthetic files only\n";
}
