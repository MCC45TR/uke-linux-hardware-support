// SPDX-License-Identifier: MIT
// Synthetic COPR-only fixtures; never reads target boot devices.
#define UKE_UPDATE_STATUS_TEST
#include "../src/update-status.cpp"
#include <cassert>
#include <cstdlib>

using namespace uke::update_status;
static bool refused(const std::string &value) {
    try { (void)saved(value); return false; } catch (const std::exception &) { return true; }
}
static std::string state(const std::string &phase="stable", const std::string &failed="-") {
    return "format=1\ngood_slot=1\ngood_release=7.3.1-1500\ngood_image="+std::string(64,'b')+
        "\nphase="+phase+"\ncandidate="+(phase=="stable"?std::string("-"):"7.3.1-1600")+
        "\ncandidate_image="+(phase=="stable"?std::string("-"):std::string(64,'c'))+
        "\nfailed="+failed+"\nstaged_boot_id="+(phase=="stable"?std::string("-"):"00000000-0000-0000-0000-000000000000")+"\n";
}
static std::array<Slot,2> slots() {
    return {Slot{Image{"7.3.1-1400","a",std::string(64,'a')},std::uint64_t(1ULL<<48)},
            Slot{Image{"7.3.1-1500","b",std::string(64,'b')},active|successful|(1ULL<<48)}};
}
int main() {
    const J empty(Json::arrayValue);
    const auto accepted=saved(state());
    auto value=report(slots(),"7.3.1-1500","b",{},1,accepted,empty);
    assert(value["complete"].asBool());
    assert(value["current_slot"].asString()=="b" && value["next_boot_slot"].asString()=="b");
    assert(!value["pending_slot_switch"].asBool());
    assert(value["kernel_approval"]["running_kernel_approved"].asBool());
    assert(value["kernel_approval"]["latest_update_approved"].asBool());
    assert(value["slots"][0]["kernel_release"].asString()=="7.3.1-1400");
    {
        auto both=slots(); both[0].boot->release="7.3.1-1500"; both[0].boot->kernel_hash="b";
        value=report(both,"7.3.1-1500","b",{},1,accepted,empty);
        assert(value["current_slot"].isNull() && value["pending_slot_switch"].isNull());
        assert(!value["complete"].asBool());
        value=report(both,"7.3.1-1500","b",1,1,accepted,empty);
        assert(value["current_slot"].asString()=="b");
    }
    value=report(slots(),"7.3.1-1500","b",0,1,accepted,empty);
    assert(value["current_slot"].isNull()); // Contradictory bootloader evidence is never guessed.
    {
        auto pending=slots(); pending[0].flags=active|(1ULL<<48); pending[1].flags=successful|(1ULL<<48);
        value=report(pending,"7.3.1-1500","b",{},0,saved(state("trial")),empty);
        assert(value["pending_slot_switch"].asBool());
        assert(value["kernel_approval"]["state"].asString()=="awaiting_confirmation");
        assert(!value["kernel_approval"]["latest_update_approved"].asBool());
        assert(value["kernel_approval"]["running_kernel_approved"].asBool());
        value=report(pending,"7.3.1-1500","b",{},1,accepted,empty);
        assert(value["pending_slot_switch"].isNull()); // UFS/GPT disagreement.
    }
    {
        auto failed=slots(); failed[1].flags=active|unbootable;
        value=report(failed,"7.3.1-1500","b",{},1,accepted,empty);
        assert(value["kernel_approval"]["latest_update_approved"].isNull());
        assert(!value["kernel_approval"]["running_kernel_approved"].asBool());
        value=report(slots(),"7.3.1-1500","b",{},1,saved(state("stable","7.3.1-6000")),empty);
        assert(value["kernel_approval"]["running_kernel_approved"].asBool());
        assert(value["kernel_approval"]["latest_update_approved"].isNull());
        assert(!value["kernel_approval"]["history_note"].isNull());
        value=report(slots(),"7.3.1-1500","b",{},1,{},empty);
        assert(value["kernel_approval"]["latest_update_approved"].isNull());
        assert(!value["complete"].asBool());
    }
    assert(refused(state()+"phase=stable\n"));
    assert(refused(state("unknown")));
    auto malformed=state(); malformed.replace(malformed.find("good_slot=1"),11,"good_slot=9"); assert(refused(malformed));
    assert(!boot_slot("quiet root=LABEL=userdata"));
    assert(boot_slot("quiet androidboot.slot_suffix=_b")==1);
    bool invalid=false; try { boot_slot("androidboot.slot_suffix=_a androidboot.slot_suffix=_b"); } catch (...) { invalid=true; } assert(invalid);
    // A sparse synthetic image exercises streaming boundaries and malformed
    // boot headers. It is created only in the remote test build directory.
    char name[]="/tmp/uke-update-status-XXXXXX"; Fd fd(mkstemp(name)); unlink(name);
    assert(!ftruncate(fd.value,capacity));
    std::array<unsigned char,4096> header{}; memcpy(header.data(),"ANDROID!",8);
    const auto put=[&](std::size_t pos,std::uint32_t n) { for(unsigned i=0;i<4;++i)header[pos+i]=static_cast<unsigned char>(n>>(i*8)); };
    put(8,131072); put(20,1584); put(40,4);
    assert(pwrite(fd.value,header.data(),header.size(),0)==ssize_t(header.size()));
    std::array<unsigned char,64> arm{}; arm[0]='M';arm[1]='Z';arm[56]=0x41;arm[57]=0x52;arm[58]=0x4d;arm[59]=0x64;
    assert(pwrite(fd.value,arm.data(),arm.size(),4096)==ssize_t(arm.size()));
    const std::string banner="Linux version 7.3.1-1500 (synthetic fixture)";
    assert(pwrite(fd.value,banner.data(),banner.size(),4096+65530)==ssize_t(banner.size()));
    const auto parsed=image(fd.value); assert(parsed.release=="7.3.1-1500");
    assert(digest_value(parsed.kernel_hash)&&digest_value(parsed.boot_hash));
    header[40]=3; assert(pwrite(fd.value,header.data(),header.size(),0)==ssize_t(header.size()));
    invalid=false;try{image(fd.value);}catch(...){invalid=true;}assert(invalid);
    value=report(slots(),"7.3.1-1500","b",{},1,accepted,empty);
    Json::StreamWriterBuilder writer; const auto serialized=Json::writeString(writer,value);
    assert(serialized.find(std::string(64,'b'))==serialized.npos);
    assert(serialized.find("staged_boot_id")==serialized.npos);
    std::cout<<"Read-only boot status, ambiguity, pending trials, approval and bounded image fixtures passed\n";
}
