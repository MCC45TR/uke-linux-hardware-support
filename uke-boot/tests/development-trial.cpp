// SPDX-License-Identifier: MIT
#define UKE_KERNEL_UPDATE_TEST
#include "../src/kernel-update.cpp"
#include "../src/userdata-update.hpp"
#include <cassert>
#include <functional>
static std::string refuses(const std::function<void()> &call) {
    try {call();}catch(const std::exception &e){return e.what();}
    assert(false);return {};
}
static const std::string boot="00000000-0000-0000-0000-000000000002";
static UserdataUpdate prior() {
    return {{uke_install::Profile::userdata,uke_install::Slot::b,"7.3.1-1500",uke_install::Phase::trial,
             "7.3.1-7200-unstable","7.3.1-7008-unstable"},std::string(64,'a'),std::string(64,'b'),
             "00000000-0000-0000-0000-000000000001"};
}
static UserdataUpdate plan(UserdataUpdate value=prior(),RollbackMode mode=RollbackMode::off,unsigned running=0,
        const std::string &release="7.3.1-7200-unstable",const std::string &current=std::string(64,'b'),
        const std::string &retained=std::string(64,'a'),const std::string &next="7.3.1-7300-unstable",
        const std::string &next_hash=std::string(64,'c'),const std::string &id=boot) {
    return replacement_trial(value,mode,running,release,current,retained,next,next_hash,id);
}
static void partial_write(int fd,const Bytes &bytes) {
    assert(pwrite(fd,bytes.data(),4096,0)==4096 && !fsync(fd));throw std::runtime_error("Injected partial write");
}
static volatile sig_atomic_t term_seen=0;
static void terminated(int) {term_seen=1;}
static void deferred_term_write(int fd,const Bytes &bytes) {
    assert(!kill(getpid(),SIGTERM));write_fd(fd,bytes);assert(!term_seen);
}
int main() {
    const auto before=prior();auto next=plan();
    assert(next.policy.phase==uke_install::Phase::staging && next.policy.good==before.policy.good &&
           next.policy.good_release==before.policy.good_release && next.good_image==before.good_image &&
           next.policy.failed==before.policy.failed && next.policy.candidate=="7.3.1-7300-unstable" && next.staged_boot_id==boot);
    uke_install::verified(next.policy);assert(uke_install::arm(next.policy)==uke_install::Slot::a);
    const auto encoded=encode(next);assert(decode(Bytes(encoded.begin(),encoded.end())).policy.good==uke_install::Slot::b);
    for(auto mode:{RollbackMode::enforced,RollbackMode::permissive})refuses([&]{plan(before,mode);});
    for(unsigned slot:{1U,2U})refuses([&]{plan(before,RollbackMode::off,slot);});
    for(auto phase:{uke_install::Phase::stable,uke_install::Phase::staging,uke_install::Phase::verified,uke_install::Phase::current_slot_trial}) {
        auto bad=before;bad.policy.phase=phase;refuses([&]{plan(bad);});
    }
    {auto bad=before;bad.policy.good=uke_install::Slot::a;refuses([&]{plan(bad);});}
    for(const auto release:{"7.3.1-1500","7.3.1-7007-unstable"})refuses([&]{plan(before,RollbackMode::off,0,release);});
    refuses([&]{plan(before,RollbackMode::off,0,before.policy.candidate,std::string(64,'d'));});
    refuses([&]{plan(before,RollbackMode::off,0,before.policy.candidate,before.candidate_image,std::string(64,'d'));});
    for(const auto candidate:{"7.3.1-7200-unstable","7.3.1-7008-unstable","7.3.1-1500","../bad"})
        refuses([&]{plan(before,RollbackMode::off,0,before.policy.candidate,before.candidate_image,before.good_image,candidate);});
    for(const auto &hash:{std::string(64,'b'),std::string(64,'a'),std::string("bad")})
        refuses([&]{plan(before,RollbackMode::off,0,before.policy.candidate,before.candidate_image,before.good_image,"7.3.1-7300-unstable",hash);});
    for(const auto &id:{before.staged_boot_id,std::string("bad")})
        refuses([&]{plan(before,RollbackMode::off,0,before.policy.candidate,before.candidate_image,before.good_image,"7.3.1-7300-unstable",std::string(64,'c'),id);});
    // Fail every forward boundary, including both readbacks. A failed image
    // restoration must leave quarantine in place and skip metadata/state acceptance.
    for(unsigned fail=0;fail<=8;++fail)for(bool recovery_failure:{false,true}) {
        unsigned step=0,writes=0,restores=0;bool blocked=false,metadata_restored=false,state_restored=false,finished=false;
        auto boundary=[&]{if(++step==fail)throw std::runtime_error("Injected boundary");};
        auto execute=[&]{replace_trial_transaction([&]{blocked=true;boundary();},[&]{boundary();},
            [&]{++writes;boundary();},[&]{boundary();},[&]{blocked=false;boundary();},[&]{boundary();finished=true;},
            [&]{blocked=true;},[&]{assert(blocked);++restores;if(recovery_failure)throw std::runtime_error("Injected recovery failure");},
            [&]{metadata_restored=true;blocked=false;},[&]{state_restored=true;});};
        if(fail>=1 && fail<=7) {
            const auto error=refuses(execute);
            assert(!finished && writes==(fail>=3?1U:0U));
            if(fail>=3 && recovery_failure) {
                assert(blocked && !metadata_restored && !state_restored && error.find("do not reboot")!=std::string::npos);
            } else assert(metadata_restored && state_restored && !blocked);
            assert(restores==(fail>=3?1U:0U));
        } else {execute();assert(finished && writes==1 && restores==0 && !metadata_restored && !state_restored);}
    }
    char temporary[]="/tmp/uke-development-trial.XXXXXX";assert(mkdtemp(temporary));const fs::path root=temporary;
    {
        Fd a(open((root/"a").c_str(),O_CREAT|O_EXCL|O_RDWR|O_CLOEXEC,0600));
        Fd b(open((root/"b").c_str(),O_CREAT|O_EXCL|O_RDWR|O_CLOEXEC,0600));
        const Bytes original(capacity,'a'),retained(capacity,'b'),candidate(capacity,'c');
        write_fd(a.value,original);write_fd(b.value,retained);
        bool blocked=false,restored=false;
        refuses([&]{replace_trial_transaction([&]{blocked=true;},[]{},[&]{
            replace_inactive_verified(a.value,b.value,candidate,retained,partial_write);
        },[]{},[]{},[]{},[&]{blocked=true;},[&]{
            assert(blocked);write_fd(a.value,original);
            assert(read_fd(a.value,capacity)==original && read_fd(b.value,capacity)==retained);
        },[&]{blocked=false;restored=true;},[]{});});
        assert(restored && !blocked && read_fd(a.value,capacity)==original && read_fd(b.value,capacity)==retained);
        struct sigaction handler{},previous{};handler.sa_handler=terminated;sigemptyset(&handler.sa_mask);
        assert(!sigaction(SIGTERM,&handler,&previous));bool finalized=false;
        {
            CriticalSignals signals;
            replace_trial_transaction([&]{blocked=true;},[]{},[&]{
                replace_inactive_verified(a.value,b.value,candidate,retained,deferred_term_write);
            },[&]{assert(!term_seen && read_fd(b.value,capacity)==retained);},[&]{blocked=false;},
            [&]{assert(!term_seen);finalized=true;},[]{assert(false);},[]{assert(false);},[]{assert(false);},[]{assert(false);});
            assert(finalized && !term_seen);
        }
        assert(term_seen && finalized && !blocked && read_fd(a.value,capacity)==candidate && read_fd(b.value,capacity)==retained);
        assert(!sigaction(SIGTERM,&previous,nullptr));
        assert(std::distance(fs::directory_iterator(root),fs::directory_iterator())==2); // No image backup file.
    }
    fs::remove_all(root);
    std::cout<<"Development trial identity/refusal, all transaction boundaries, failed-restoration quarantine, actual partial-write recovery and deferred SIGTERM passed\n";
}
