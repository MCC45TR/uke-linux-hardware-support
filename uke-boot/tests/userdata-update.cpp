// SPDX-License-Identifier: MIT
#define UKE_KERNEL_UPDATE_TEST
#include "../src/kernel-update.cpp"
#include "../src/userdata-update.hpp"
#include <cassert>
#include <functional>
static void reject(const std::function<void()> &call){bool refused=false;try{call();}catch(const std::exception&){refused=true;}assert(refused);}
static void put32(Bytes &bytes,size_t offset,uint32_t value){for(unsigned i=0;i<4;++i)bytes.at(offset+i)=value>>(i*8);}
static void capability(Bytes &bytes,size_t offset=page+256) {
    put32(bytes,offset,4);put32(bytes,offset+4,53);put32(bytes,offset+8,0x554b4501);
    memcpy(bytes.data()+offset+12,"UKE\0root-selector-v1;userdata=f2fs;slot=a,b;vrr-min=0,30",57);
}
static Bytes image(const std::string &release) {
    Bytes bytes(capacity);memcpy(bytes.data(),"ANDROID!",8);put32(bytes,8,512);put32(bytes,20,1584);put32(bytes,40,4);
    const std::string selector="senemos.root=userdata";memcpy(bytes.data()+44,selector.data(),selector.size());
    bytes[page]='M';bytes[page+1]='Z';put32(bytes,page+16,512);put32(bytes,page+24,10);put32(bytes,page+56,0x644d5241);
    put32(bytes,page+60,64);put32(bytes,page+64,0x4550);bytes[page+68]=0x64;bytes[page+69]=0xaa;
    const auto banner="Linux version "+release+" root=PARTLABEL=linux rw msm.o82_vrr_test_min_hz=";
    memcpy(bytes.data()+page+128,banner.data(),banner.size());return bytes;
}
int main() {
    for(const std::string global:{"0","1"})for(const std::string profile:{"0","1"})
        assert(automatic_updates("# Local policy\n\nautomatic="+global+"\n","automatic="+profile+"\nvrr_min_hz=30\n")==
               (global=="1" && profile=="1"));
    for(const std::string global:{"","automatic=2\n","automatic=true\n","automatic=1 \n","automatic=1\nautomatic=1\n","automatic=0\nextra=1\n"})
        reject([&]{automatic_updates(global,"automatic=1\nvrr_min_hz=30\n");});
    for(const std::string profile:{"","automatic=2\nvrr_min_hz=30\n","automatic=1\n","automatic=1\nvrr_min_hz=31\n","automatic=1\nautomatic=1\nvrr_min_hz=30\n"})
        reject([&]{automatic_updates("automatic=0\n",profile);});
    char temporary[]="/tmp/uke-userdata-state.XXXXXX";assert(mkdtemp(temporary));const fs::path root=temporary;
    UserdataUpdate saved{{uke_install::Profile::userdata,uke_install::Slot::b,"7.2.11-14",uke_install::Phase::stable,{},{}},std::string(64,'a'),{}, {}};
    save_update(root/"state",saved);assert(encode(decode(file(root/"state")))==encode(saved));
    assert(uke_install::stage(saved.policy,uke_install::Slot::b,"7.2.11-14","7.2.11-16")==uke_install::Slot::a);
    saved.candidate_image=std::string(64,'b');saved.staged_boot_id="00000000-1234-5678-aaaa-ffffffffffff";save_update(root/"state",saved);
    auto restored=decode(file(root/"state"));reject([&]{uke_install::arm(restored.policy);});
    uke_install::verified(saved.policy);save_update(root/"state",saved);(void)uke_install::arm(saved.policy);save_update(root/"state",saved);
    restored=decode(file(root/"state"));reject([&]{uke_install::accept(restored.policy,uke_install::Slot::a,"7.2.11-15",true);});
    uke_install::returned(restored.policy,uke_install::Slot::b,"7.2.11-14");restored.candidate_image.clear();restored.staged_boot_id.clear();save_update(root/"state",restored);
    reject([&]{uke_install::stage(restored.policy,uke_install::Slot::b,"7.2.11-14","7.2.11-16");});
    uke_install::accept(saved.policy,uke_install::Slot::a,"7.2.11-16",true);saved.good_image=saved.candidate_image;saved.candidate_image.clear();saved.staged_boot_id.clear();save_update(root/"state",saved);
    assert(decode(file(root/"state")).policy.good==uke_install::Slot::a);
    save_new(root/"state.new",Bytes{'s','t','a','l','e'});save_update(root/"state",saved);
    fs::create_symlink("state",root/"state.new");reject([&]{save_update(root/"state",saved);});fs::remove(root/"state.new");
    fs::create_hard_link(root/"state",root/"state.new");reject([&]{save_update(root/"state",saved);});fs::remove(root/"state.new");
    auto malformed=encode(saved)+"format=1\n";reject([&]{decode(Bytes(malformed.begin(),malformed.end()));});
    auto other=saved;other.policy.candidate="stale";reject([&]{encode(other);});other=saved;other.good_image="bad";reject([&]{encode(other);});
    assert(valid_boot_id("00000000-1234-5678-aaaa-ffffffffffff") && !valid_boot_id("00000000-1234-5678-aaaa-fffffffffffz"));
    fs::create_symlink("state",root/"unsafe");reject([&]{save_update(root/"unsafe",saved);});fs::remove(root/"unsafe");
    fs::create_hard_link(root/"state",root/"unsafe");reject([&]{save_update(root/"unsafe",saved);});fs::remove(root/"unsafe");
    std::array<Bytes,2> boots{image("7.2.11-14"),image("7.2.11-16")};
    const auto running=boot_kernel(boots[1]);assert(identify_slot("root=PARTLABEL=userdata",boots,running)==1);
    require_current_b("root=PARTLABEL=userdata",boots,running);
    reject([&]{require_current_b("root=PARTLABEL=userdata",boots,boot_kernel(boots[0]));});
    assert(identify_slot("androidboot.slot_suffix=_b",boots,running)==1);
    reject([&]{identify_slot("androidboot.slot_suffix=_a",boots,running);});
    boots[0]=boots[1];reject([&]{identify_slot("",boots,running);});assert(identify_slot("androidboot.slot_suffix=_b",boots,running)==1);
    reject([&]{require_current_b("",boots,running);});
    const std::map<std::string,std::string> manifest{{"root_selector_capability","uke-elfnote-v1"}};
    assert(userdata_vrr(boots[1],false,{})==boots[1]);
    reject([&]{userdata_vrr(boots[1],true,manifest);}); // The old executable literal is not a capability.
    auto capable=boots[1];capability(capable);const auto vrr=userdata_vrr(capable,true,manifest);
    assert(boot_kernel(vrr)==boot_kernel(capable) && std::string(reinterpret_cast<const char*>(vrr.data()+44))=="senemos.root=userdata msm.o82_vrr_test_min_hz=30");
    reject([&]{userdata_vrr(vrr,true,manifest);});
    reject([&]{userdata_vrr(capable,true,{});});
    reject([&]{userdata_vrr(capable,true,{{"root_selector_capability","unknown"}});});
    for(const size_t offset:{size_t(256),size_t(260),size_t(264),size_t(268),size_t(272),size_t(324),size_t(327)}) {
        auto corrupt=capable;corrupt[page+offset]^=1;reject([&]{userdata_vrr(corrupt,true,manifest);});
    }
    auto duplicate=capable;capability(duplicate,page+336);reject([&]{userdata_vrr(duplicate,true,manifest);});
    auto unaligned=boots[1];capability(unaligned,page+257);reject([&]{userdata_vrr(unaligned,true,manifest);});
    auto truncated=boots[1];capability(truncated,page+460);reject([&]{userdata_vrr(truncated,true,manifest);});
    fs::remove_all(root);
    std::cout<<"Userdata durable states, staged/verified/trial gates, healthy acceptance/fallback, stale/symlink/hardlink refusals, unique running Image identity and bounded VRR derivation passed; regular files only\n";
}
