// SPDX-License-Identifier: MIT
// Actual updater functions, synthetic boot files; no device or firmware operation.
#define UKE_KERNEL_UPDATE_TEST
#include "../src/kernel-update.cpp"
#include "../src/userdata-update.hpp"
#include <cassert>
#include <functional>

static void refuse(const std::function<void()>& action) {
    bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}
    assert(rejected);
}
static void put(Bytes& b,size_t offset,uint32_t value) {
    for(unsigned i=0;i<4;++i)b.at(offset+i)=value>>(i*8);
}
static void note(Bytes& b,size_t offset,uint32_t type,const std::string& descriptor) {
    put(b,offset,4);put(b,offset+4,descriptor.size()+1);put(b,offset+8,type);
    memcpy(b.data()+offset+12,"UKE",4);
    memcpy(b.data()+offset+16,descriptor.c_str(),descriptor.size()+1);
}
static Bytes sample(bool new_capability) {
    Bytes b(capacity);memcpy(b.data(),"ANDROID!",8);
    put(b,8,2048);put(b,20,1584);put(b,40,4);
    memcpy(b.data()+44,"senemos.root=userdata",20);
    b[page]='M';b[page+1]='Z';put(b,page+16,2048);put(b,page+24,10);
    put(b,page+56,0x644d5241);put(b,page+60,64);put(b,page+64,0x4550);
    b[page+68]=0x64;b[page+69]=0xaa;
    note(b,page+256,0x554b4501,"root-selector-v2;userdata=f2fs,ext4,xfs,btrfs;slot=a,b;vrr-min=0,30");
    if(new_capability)note(b,page+512,0x554b4503,"super-root-v1;role=rescue;root=dm-uke-rescue;readonly;noresume;seed=4GiB");
    return b;
}
int main() {
    using namespace uke_boot_profile;
    const std::map<std::string,std::string> manifest{{"root_selector_capability","uke-elfnote-v2"}};
    const auto legacy=sample(false),modern=sample(true);
    const auto raw=boot_kernel(modern);
    assert(!capable(boot_kernel(legacy))&&capable(raw));
    assert(userdata_vrr(legacy,false,manifest,"ext4",false,0)==legacy);
    refuse([&]{(void)userdata_vrr(legacy,false,manifest,"ext4",true,0);});
    refuse([&]{(void)derive_slot_image(legacy,0);});
    for(bool super:{false,true})for(bool vrr:{false,true})for(unsigned slot:{0u,1u}) {
        const auto bytes=userdata_vrr(modern,vrr,manifest,"ext4",super,slot);
        const auto profile=parse(std::string(reinterpret_cast<const char*>(bytes.data()+44)));
        assert(profile.userdata&&profile.super==super&&profile.vrr30==vrr&&profile.slot==slot);
        assert(boot_kernel(bytes)==raw);
        slot_matches(profile,slot);refuse([&]{slot_matches(profile,1-slot);});
        const auto other=derive_slot_image(bytes,1-slot);
        assert(boot_kernel(other)==raw);
        const auto changed=parse(std::string(reinterpret_cast<const char*>(other.data()+44)));
        assert(changed.slot==1-slot&&changed.super==super&&changed.vrr30==vrr);
    }
    const auto a=userdata_vrr(modern,true,manifest,"ext4",true,0);
    const auto b=userdata_vrr(modern,true,manifest,"ext4",true,1);
    const std::array<Bytes,2> both{a,b};
    assert(identify_slot("root=PARTLABEL=userdata rw androidboot.slot_suffix=_a",both,raw)==0);
    assert(identify_slot("root=PARTLABEL=userdata rw androidboot.slot_suffix=_b",both,raw)==1);
    refuse([&]{(void)identify_slot("root=PARTLABEL=userdata rw",both,raw);});
    refuse([&]{(void)identify_slot("androidboot.slot_suffix=_a androidboot.slot_suffix=_b",both,raw);});
    const std::array<Bytes,2> misplaced{b,a};
    refuse([&]{(void)identify_slot("androidboot.slot_suffix=_a",misplaced,raw);});
    auto broken=modern;broken[page+512+8]=2;
    assert(!capable(boot_kernel(broken)));
    refuse([&]{(void)userdata_vrr(broken,false,manifest,"ext4",true,0);});
    broken=modern;note(broken,page+1024,0x554b4503,"super-root-v1;role=rescue;root=dm-uke-rescue;readonly;noresume;seed=4GiB");
    assert(!capable(boot_kernel(broken)));
    refuse([&]{(void)derive_slot_image(broken,0);});
    broken=modern;memmove(broken.data()+page+513,broken.data()+page+512,100);broken[page+512]=0;
    assert(!capable(boot_kernel(broken)));
    refuse([&]{(void)derive_slot_image(broken,0);});
    for(const std::string text:{"senemos.root=userdata rw","senemos.root=userdata senemos.role=rescue",
        "senemos.root=userdata senemos.super=seed4-v2","senemos.root=userdata senemos.super=seed4-v1 senemos.super=seed4-v1",
        "senemos.root=userdata androidboot.slot_suffix=_a androidboot.slot_suffix=_a",
        "senemos.root=userdata androidboot.slot_suffix=_c","senemos.root=userdata ",
        "senemos.root=userdata androidboot.slot_suffix=_a senemos.super=seed4-v1"}) {
        refuse([&]{(void)parse(text);});
        broken=modern;std::fill(broken.begin()+44,broken.begin()+1580,0);std::copy(text.begin(),text.end(),broken.begin()+44);
        refuse([&]{(void)boot_kernel(broken);});
    }
    assert(!requested("root=PARTLABEL=userdata rw"));
    assert(requested("root=PARTLABEL=userdata rw quiet senemos.super=seed4-v1\n"));
    for(const std::string text:{"root=PARTLABEL=userdata ro senemos.super=seed4-v1",
        "root=PARTLABEL=userdata rw ro senemos.super=seed4-v1",
        "root=PARTLABEL=linux rw senemos.super=seed4-v1",
        "root=PARTLABEL=userdata rw senemos.role=rescue senemos.super=seed4-v1",
        "root=PARTLABEL=userdata rw senemos.super=unknown",
        "root=PARTLABEL=userdata rw senemos.super=seed4-v1 senemos.super=seed4-v1"})
        refuse([&]{(void)requested(text);});
    std::cout<<"PASS: actual super boot updater; legacy fallback, profile preservation, both-slot identity, capability and malformed-header refusals; synthetic files only\n";
}
