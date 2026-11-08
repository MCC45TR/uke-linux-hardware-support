// SPDX-License-Identifier: MIT
// Actual updater guards with regular files only; no hardware/reboot providers.
#define UKE_KERNEL_UPDATE_TEST
#include "../src/kernel-update.cpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <initializer_list>
#include <utility>

static void put(const fs::path &path,const std::string &value) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path);output<<value;assert(output.good());
}
int main() {
    char temporary[]="/tmp/uke-critical-userdata-XXXXXX";assert(mkdtemp(temporary));
    const fs::path root=temporary;
    const std::string boot="01234567-89ab-cdef-0123-456789abcdef";
    const std::string good_release="7.3.3-7-unstable",candidate="7.3.3-9-unstable";
    const std::string good_hash(64,'a'),candidate_hash(64,'b');
    unsigned cases=0;
    for(const auto good:{uke_install::Slot::a,uke_install::Slot::b}) {
        UserdataUpdate retained{{uke_install::Profile::userdata,good,good_release,
            uke_install::Phase::stable,{},{}},good_hash,{}, {}};
        const unsigned running=unsigned(uke_install::stage(retained.policy,good,good_release,candidate));
        retained.candidate_image=candidate_hash;
        retained.staged_boot_id="11234567-89ab-cdef-0123-456789abcdef";
        uke_install::verified(retained.policy);(void)uke_install::arm(retained.policy);
        save_update(root/"retained",retained);
        const auto observed=decode(file(root/"retained"));
        const auto matches=[&](const UserdataUpdate &value,const std::string &recorded=std::string{}) {
            return critical_retained_pair_matches(value,recorded.empty()?boot+"\n":recorded,
                boot,running,candidate,candidate_hash,good_hash);
        };
        assert(matches(observed));cases++;
        assert(!matches(observed,"11234567-89ab-cdef-0123-456789abcdef\n"));cases++;
        assert(!matches(observed,boot));cases++;
        assert(!matches(observed,boot+"\nextra\n"));cases++;
        for(const auto phase:{uke_install::Phase::stable,uke_install::Phase::staging,
                             uke_install::Phase::verified,uke_install::Phase::current_slot_trial}) {
            auto changed=observed;changed.policy.phase=phase;assert(!matches(changed));cases++;
        }
        auto changed=observed;changed.policy.candidate=good_release;assert(!matches(changed));cases++;
        changed=observed;changed.candidate_image=good_hash;assert(!matches(changed));cases++;
        changed=observed;changed.good_image=candidate_hash;assert(!matches(changed));cases++;
        changed=observed;changed.policy.good=static_cast<uke_install::Slot>(running);
        assert(!matches(changed));cases++;
        assert(!critical_retained_pair_matches(observed,boot+"\n",boot,2,
            candidate,candidate_hash,good_hash));cases++;
        assert(!critical_retained_pair_matches(observed,"bad\n","bad",running,
            candidate,candidate_hash,good_hash));cases++;
        auto accepted=observed;
        uke_install::accept(accepted.policy,static_cast<uke_install::Slot>(running),candidate,true);
        assert(!matches(accepted));cases++;
        auto returned=observed;
        uke_install::returned(returned.policy,good,good_release);
        returned.candidate_image.clear();returned.staged_boot_id.clear();
        save_update(root/"returned",returned);
        auto again=decode(file(root/"returned"));
        assert(again.policy.failed==candidate);cases++;
        bool refused=false;
        try{(void)uke_install::stage(again.policy,good,good_release,candidate);}
        catch(const std::exception &){refused=true;}
        assert(refused);cases++;
    }
    const auto power=root/"sys/class/power_supply";
    const auto independent=power/"ucsi-source-psy-pmic_glink.ucsi.01";
    assert(!fallback_metadata_power(true,root));cases++;
    put(independent/"type","USB\n");put(independent/"online","1\n");
    assert(fallback_metadata_power(true,root));cases++;
    assert(!fallback_metadata_power(false,root));cases++;
    for(const auto online:{"0\n","1 trailing\n","1\n0\n",""}) {
        put(independent/"online",online);assert(!fallback_metadata_power(true,root));cases++;
    }
    put(independent/"online","1\n");put(independent/"type","Battery\n");
    assert(!fallback_metadata_power(true,root));cases++;
    put(independent/"type","USB\n");put(independent/"online","0\n");
    put(power/"battery/type","Battery\n");
    for(const auto &[value,expected]:std::initializer_list<std::pair<std::string,bool>>{
            {"0\n",false},{"49\n",false},{"50\n",true},{"100\n",true},{"101\n",false}}) {
        put(power/"battery/capacity",value);
        assert(fallback_metadata_power(false,root)==expected);cases++;
    }
    put(power/"battery/capacity","unavailable\n");put(independent/"online","1\n");
    assert(fallback_metadata_power(true,root));cases++;
    assert(!fallback_metadata_power(false,root));cases++;
    fs::remove_all(root);
    std::cout<<"Actual critical updater guards: "<<cases
             <<" retained-pair/durable-state/failed-retry/power scenarios passed; regular files only.\n";
}
