// SPDX-License-Identifier: MIT
// Real updater functions, ordinary files only; syscall errors are injected at link time.
#define UKE_KERNEL_UPDATE_TEST
#include "../src/kernel-update.cpp"
#include "../src/userdata-update.hpp"
#include <cassert>
#include <functional>

extern "C" int __real_fsync(int);
extern "C" ssize_t __real_pwrite(int,const void*,size_t,off_t);
extern "C" int __real_renameat(int,const char*,int,const char*);
static bool recording=false,crash=false,short_write=false;
static unsigned step=0,fail_at=0;
static int injected_errno=EIO;
static std::vector<std::pair<dev_t,ino_t>> synced_directories;
static bool fault() {
    if(!recording || ++step!=fail_at)return false;
    if(crash){kill(getpid(),SIGKILL);_exit(99);}
    errno=injected_errno;return true;
}
extern "C" int __wrap_fsync(int fd) {
    if(recording) {
        struct stat st{};assert(!fstat(fd,&st));
        if(S_ISDIR(st.st_mode))synced_directories.emplace_back(st.st_dev,st.st_ino);
    }
    if(fault())return -1;
    return __real_fsync(fd);
}
extern "C" ssize_t __wrap_pwrite(int fd,const void *data,size_t size,off_t offset) {
    if(fault())return -1;
    if(recording && short_write && step==1)size=std::min(size,size_t(8));
    return __real_pwrite(fd,data,size,offset);
}
extern "C" int __wrap_renameat(int from,const char *name,int to,const char *next) {
    if(fault())return -1;
    return __real_renameat(from,name,to,next);
}
static void start(unsigned at=0,bool killed=false) {
    step=0;fail_at=at;crash=killed;short_write=false;injected_errno=EIO;
    synced_directories.clear();recording=true;
}
static void refused(const std::function<void()> &call) {
    bool failed=false;try{call();}catch(const std::exception&){failed=true;}
    recording=false;assert(failed);
}
static bool synced(const fs::path &path) {
    struct stat st{};assert(!stat(path.c_str(),&st));
    return std::find(synced_directories.begin(),synced_directories.end(),std::pair(st.st_dev,st.st_ino))!=synced_directories.end();
}
int main() {
    char pattern[]="/tmp/uke-update-durability.XXXXXX";assert(mkdtemp(pattern));const fs::path root=pattern;
    start();{auto fd=directory(root/"baseline/transaction",true);}recording=false;
    const auto directory_steps=step;
    // A synced child alone cannot establish that its name survives in its parent.
    assert(directory_steps>0 && synced(root) && synced(root/"baseline"));
    for(unsigned point=1;point<=directory_steps;++point) {
        const auto path=root/("failure-"+std::to_string(point))/"transaction";
        start(point);refused([&]{auto fd=directory(path,true);save_new(path/"must-not-exist",Bytes{'x'});});
        assert(!fs::exists(path/"must-not-exist"));
        // Retry must synchronize existing links too: the prior mkdir may have
        // succeeded before fsync failed or the updater was terminated.
        start();{auto fd=directory(path,true);}recording=false;
        assert(step==directory_steps && synced(root) && synced(path.parent_path()));
    }
    const auto interrupted=root/"interrupted/transaction";
    const auto creator=fork();assert(creator>=0);
    if(!creator){start(directory_steps,true);auto fd=directory(interrupted,true);_exit(99);}
    int status=0;assert(waitpid(creator,&status,0)==creator && WIFSIGNALED(status) && WTERMSIG(status)==SIGKILL);
    start();{auto fd=directory(interrupted,true);}recording=false;
    assert(step==directory_steps && synced(root) && synced(interrupted.parent_path()));

    UserdataUpdate old{{uke_install::Profile::userdata,uke_install::Slot::b,"7.3.0-54",uke_install::Phase::stable,{},{}},std::string(64,'a'),{}, {}};
    auto next=old;uke_install::stage(next.policy,uke_install::Slot::b,"7.3.0-54","7.3.0-55");
    next.candidate_image=std::string(64,'b');next.staged_boot_id="00000000-1234-5678-aaaa-ffffffffffff";
    const auto state=root/"userdata.state";save_update(state,old);
    start();save_update(state,next);recording=false;const auto state_steps=step;
    assert(state_steps==4); // pwrite, file fsync, renameat, parent fsync.
    for(const bool killed:{false,true})for(unsigned point=1;point<=state_steps;++point) {
        save_update(state,old);
        if(killed) {
            const auto child=fork();assert(child>=0);
            if(!child){start(point,true);save_update(state,next);_exit(99);}
            assert(waitpid(child,&status,0)==child && WIFSIGNALED(status) && WTERMSIG(status)==SIGKILL);
        } else {start(point);refused([&]{save_update(state,next);});}
        const auto observed=encode(decode(file(state)));
        assert(observed==encode(old)||observed==encode(next));
        save_update(state,next);assert(encode(decode(file(state)))==encode(next));
        assert(!fs::exists(root/"userdata.state.new"));
    }
    save_update(state,old);start(2);short_write=true;injected_errno=ENOSPC;
    refused([&]{save_update(state,next);});assert(encode(decode(file(state)))==encode(old));
    save_update(state,next);assert(encode(decode(file(state)))==encode(next));
    fs::remove_all(root);
    std::cout<<"Directory-link sync/refusal/retry, process interruption, four state-write error/crash boundaries and partial ENOSPC recovery passed; regular files only, not power-loss qualification\n";
}
