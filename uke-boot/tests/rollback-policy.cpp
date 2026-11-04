// SPDX-License-Identifier: MIT
#define UKE_KERNEL_UPDATE_TEST
#include "../src/kernel-update.cpp"
#include <cassert>
#include <functional>
static void rejects(const std::function<void()> &f) {
    bool refused=false;try{f();}catch(const std::exception &){refused=true;}assert(refused);
}
int main() {
    char temporary[]="/tmp/uke-rollback-policy.XXXXXX";assert(mkdtemp(temporary));
    const fs::path config=fs::path(temporary)/"rollback.conf";
    assert(read_rollback_policy(config)==RollbackMode::enforced);
    for(const auto mode:{RollbackMode::off,RollbackMode::permissive,RollbackMode::enforced}) {
        set_rollback_policy(mode,config);
        assert(read_rollback_policy(config)==mode);
        assert(rollback_mode(rollback_name(mode))==mode);
        assert(!rollback_retry(mode,false));
        assert(rollback_retry(mode,true)==(mode!=RollbackMode::enforced));
    }
    rejects([]{rollback_mode("disabled");});
    rejects([]{rollback_mode("off\n");});
    {std::ofstream bad(config);bad<<"mode=off\nmode=enforced\n";}
    rejects([&]{read_rollback_policy(config);});
    fs::remove(config);fs::create_symlink("absent",config);
    rejects([&]{read_rollback_policy(config);});
    fs::remove(config);
    uke_install::Update stable{uke_install::Profile::userdata,uke_install::Slot::b,"7.3.1-1500",
                              uke_install::Phase::stable,{},"7.3.1-7008-unstable"};
    rejects([&]{auto copy=stable;uke_install::stage(copy,copy.good,copy.good_release,copy.failed);});
    for(const auto mode:{RollbackMode::off,RollbackMode::permissive}) {
        auto copy=stable;
        assert(uke_install::stage(copy,copy.good,copy.good_release,copy.failed,rollback_retry(mode,true))==uke_install::Slot::a);
        assert(copy.failed==stable.failed); // Preserve the failed-release history.
        rejects([&]{auto same=stable;uke_install::stage(same,same.good,same.good_release,same.good_release,true);});
        rejects([&]{auto wrong=stable;uke_install::stage(wrong,uke_install::Slot::a,wrong.good_release,wrong.failed,true);});
    }
    fs::remove_all(temporary);
    std::cout<<"Rollback defaults, durable modes, invalid policy refusal and explicit retry guards passed\n";
}
