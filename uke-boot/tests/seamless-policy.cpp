// SPDX-License-Identifier: MIT
#define UKE_KERNEL_UPDATE_TEST
#include "../src/kernel-update.cpp"
#include <cassert>
#include <functional>
static void rejects(const std::function<void()> &f) {
    bool refused=false;try{f();}catch(const std::exception&){refused=true;}assert(refused);
}
static void text(const fs::path &path,const std::string &value) {std::ofstream(path)<<value;}
int main() {
    char temp[]="/tmp/uke-seamless-policy.XXXXXX";assert(mkdtemp(temp));
    const fs::path root=temp,config=root/"etc",state=root/"state";
    fs::create_directories(config);fs::create_directories(state);
    const auto global=config/"kernel-update.conf",profile=config/"userdata-update.conf",rollback=config/"rollback.conf";
    for(const std::string mode:{"off","permissive","enforced"}) {
        text(global,"# Legacy temporary opt-out\nautomatic=0\n");
        text(profile,"automatic=0\nvrr_min_hz=30\n");text(rollback,"mode="+mode+"\n");
        const auto before=file(rollback);struct stat saved{};assert(!stat(rollback.c_str(),&saved));
        assert(enable_seamless_policy(config,state));
        assert(policy_text(global)=="automatic=1\n" && policy_text(profile)=="automatic=1\nvrr_min_hz=30\n");
        assert(file(rollback)==before && read_rollback_policy(rollback)==rollback_mode(mode));
        assert(!enable_seamless_policy(config,state));
        struct stat after{};assert(!stat(rollback.c_str(),&after));assert(saved.st_ino==after.st_ino && saved.st_mode==after.st_mode);
    }
    text(global,"# Enabled user policy\nautomatic=1\n");text(profile,"automatic=1\nvrr_min_hz=0\n");
    assert(!enable_seamless_policy(config,state));
    assert(policy_text(global)=="# Enabled user policy\nautomatic=1\n" && policy_text(profile)=="automatic=1\nvrr_min_hz=0\n");
    text(state/"automatic-policy-v1",policy_prepared);
    text(global,"automatic=0\n");text(profile,"automatic=1\nvrr_min_hz=30\n");text(rollback,"mode=off\n");
    assert(enable_seamless_policy(config,state) && automatic_policy_ready(state));
    assert(policy_text(state/"automatic-policy-v1")==policy_complete && policy_text(rollback)=="mode=off\n");
    assert(!enable_seamless_policy(config,state));
    text(global,"automatic=0\n");text(profile,"automatic=0\n");assert(enable_seamless_policy(config,state));
    assert(policy_text(profile)=="automatic=1\nvrr_min_hz=0\n");
    text(profile,"automatic=1\nvrr_min_hz=17\n");rejects([&]{enable_seamless_policy(config,state);});
    text(profile,"automatic=1\nvrr_min_hz=0\n");fs::remove(global);fs::create_symlink(profile,global);
    rejects([&]{enable_seamless_policy(config,state);});

    uke_install::Update saved{uke_install::Profile::userdata,uke_install::Slot::b,"7.3.1-1500",
                              uke_install::Phase::trial,"7.3.1-7200-unstable","7.3.1-7008-unstable"};
    const std::string prior="aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa",boot="bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb";
    const auto plan=[&](RollbackMode mode,const std::string &running,const std::string &selected) {
        return seamless_stage(mode,saved,prior,boot,running,selected);
    };
    assert(plan(RollbackMode::off,saved.candidate,saved.candidate)==SeamlessStage::current);
    assert(plan(RollbackMode::off,saved.candidate,"7.3.1-7300-unstable")==SeamlessStage::replace_trial);
    assert(plan(RollbackMode::enforced,saved.candidate,"7.3.1-7300-unstable")==SeamlessStage::apply);
    assert(plan(RollbackMode::permissive,saved.candidate,"7.3.1-7300-unstable")==SeamlessStage::apply);
    rejects([&]{plan(RollbackMode::off,saved.candidate,saved.failed);});
    assert(plan(RollbackMode::off,saved.candidate,"7.3.1-7100-unstable")==SeamlessStage::older);
    assert(pending_development_reboot(RollbackMode::off,saved,boot,boot,"7.3.1-7100-unstable"));
    assert(!pending_development_reboot(RollbackMode::enforced,saved,boot,boot,"7.3.1-7100-unstable"));
    assert(!pending_development_reboot(RollbackMode::off,saved,prior,boot,saved.candidate));
    saved.phase=uke_install::Phase::stable;saved.candidate.clear();
    assert(plan(RollbackMode::off,"7.3.1-1500","7.3.1-7300-unstable")==SeamlessStage::apply);
    assert(unstable_version("7.3.1-10000-unstable")>unstable_version("7.3.1-9999-unstable"));
    assert(unstable_version("7.4.0-1-unstable")>unstable_version("7.3.1-99999-unstable"));
    rejects([]{unstable_version("7.3.1-7200.1-unstable");});
    rejects([]{unstable_version("7.3.1-18446744073709551616-unstable");});
    assert(skip_off_acceptance(RollbackMode::off,true));rejects([]{skip_off_acceptance(RollbackMode::off,false);});
    assert(!skip_off_acceptance(RollbackMode::enforced,true));assert(!skip_off_acceptance(RollbackMode::permissive,true));
    fs::remove_all(root);
    std::cout<<"Seamless enablement, repeated-install rollback persistence, display-policy preservation, unsafe-file refusal, off-mode staging and failed/downgrade guards passed\n";
}
