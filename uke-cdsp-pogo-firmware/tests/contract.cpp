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
    Bytes auth_mdt(52+9*32);memcpy(auth_mdt.data(),"\177ELF",4);
    auth_mdt[4]=auth_mdt[5]=auth_mdt[6]=1;put16(auth_mdt,18,183);
    put32(auth_mdt,28,52);put16(auth_mdt,40,52);put16(auth_mdt,42,32);put16(auth_mdt,44,9);
    std::map<std::string,Bytes> auth_parts;Bytes joined_expected(auth_bytes);
    for(unsigned i=0;i<9;i++) {
        const auto at=i?4096+i*128:0;const auto size=i?64:340;
        put32(auth_mdt,52+i*32,1);put32(auth_mdt,52+i*32+4,at);put32(auth_mdt,52+i*32+16,size);
        char part_name[16];snprintf(part_name,sizeof(part_name),"DEVAUTH.B%02u",i);
        auth_parts[part_name]=Bytes(size,static_cast<unsigned char>(i+1));
        std::fill(joined_expected.begin()+at,joined_expected.begin()+at+size,i+1);
    }
    const auto auth_read=[&](const auto &name,size_t maximum){assert(maximum==auth_bytes);return auth_parts.at(name);};
    assert(join_auth_parts(auth_mdt,auth_read)==joined_expected);
    Bytes auth64(64+9*56);memcpy(auth64.data(),"\177ELF",4);
    auth64[4]=2;auth64[5]=auth64[6]=1;put16(auth64,18,183);put32(auth64,32,64);
    put16(auth64,52,64);put16(auth64,54,56);put16(auth64,56,9);
    for(unsigned i=0;i<9;i++) {
        put32(auth64,64+i*56,1);put32(auth64,64+i*56+8,i?4096+i*128:0);
        put32(auth64,64+i*56+32,i?64:340);
    }
    assert(join_auth_parts(auth64,auth_read)==joined_expected);
    auto bad64=auth64;put32(bad64,64+56+12,1);refused([&]{join_auth_parts(bad64,auth_read);});
    assert(!admitted_auth(joined_expected));
    auto broken_auth=auth_mdt;put32(broken_auth,52+32+4,auth_bytes);refused([&]{join_auth_parts(broken_auth,auth_read);});
    broken_auth=auth_mdt;put16(broken_auth,44,8);refused([&]{join_auth_parts(broken_auth,auth_read);});
    broken_auth=auth_mdt;put32(broken_auth,52+32+4,0);refused([&]{join_auth_parts(broken_auth,auth_read);});
    auth_parts["DEVAUTH.B01"].pop_back();refused([&]{join_auth_parts(auth_mdt,auth_read);});
    auth_parts["DEVAUTH.B01"].push_back(2);
    broken_auth=auth_mdt;put32(broken_auth,52+32*2+4,4096+128);
    auth_parts["DEVAUTH.B02"]=auth_parts["DEVAUTH.B01"];
    assert(join_auth_parts(broken_auth,auth_read).size()==auth_bytes);
    refused([&]{auth_field(auth_mdt,0,9);});
    const fs::path controller="/sys/devices/platform/soc/1d84000.ufshc";
    assert(below(controller/"host0/target0:0:0/0:0:0:4/block/sde/sde6",controller));
    assert(below(controller/"host0/target0:0:0/0:0:0:0/block/sda/sda31",controller));
    assert(!below("/sys/devices/platform/soc/1d84000.ufshc-other/sda31",controller));
    assert(!below(controller/"../other/sda31",controller));
    assert(!below(controller,controller));
    char name[]="/tmp/uke-fw-test-XXXXXX";const auto root=mkdtemp(name);assert(root);
    const fs::path dir=root;Fd image(open((dir/"fat.img").c_str(),O_RDWR|O_CREAT|O_EXCL,0600));
    auto bytes=fat16();write_image(image.value,bytes);
    Fat fat(image.value,bytes.size());const auto payload=fat.image_file("CDSP.MDT");
    assert(payload.size()==700);for(unsigned i=0;i<payload.size();i++)assert(payload[i]==(i&255));
    refused([&]{cdsp_family(fat);});
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
    std::cout<<"PASS: actual read-only FAT, nine-part TA assembly/overlaps/bounds, exact identity refusal and atomic no-replace publication; synthetic files only\n";
}
