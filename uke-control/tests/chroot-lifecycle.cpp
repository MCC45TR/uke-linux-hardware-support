// SPDX-License-Identifier: MIT
// Trusted native fixture only. No target distribution code, mounts or disks.
#include "../src/chroot.cpp"
#include <cassert>
#include <chrono>
#include <thread>
using namespace uke::chroot_manager;
static pid_t host_pid() {
    std::istringstream lines(file("/proc/self/status"));std::string line;
    while(std::getline(lines,line))if(line.starts_with("NSpid:")){std::istringstream row(line.substr(6));pid_t id=0;row>>id;return id;}
    assert(false);return 0;
}
static void target(int output,bool wait) {
    int ready[2];assert(pipe(ready)==0);
    const auto detached=fork();assert(detached>=0);
    if(!detached){close(ready[0]);assert(setsid()>=0);const auto id=host_pid();assert(::write(output,&id,sizeof(id))==sizeof(id));assert(::write(ready[1],"x",1)==1);close(ready[1]);for(;;)pause();}
    close(ready[1]);char byte;assert(::read(ready[0],&byte,1)==1);close(ready[0]);
    if(wait)for(;;)pause();
    _exit(23);
}
int main(){
    assert(prctl(PR_SET_CHILD_SUBREAPER,1)==0);
    for(bool killed:{false,true}) {
        int ids[2];assert(pipe(ids)==0);const auto manager=fork();assert(manager>=0);
        if(!manager){
            close(ids[0]);
            if(unshare(CLONE_NEWPID)!=0){std::cerr<<"PID namespace test unavailable: "<<std::strerror(errno)<<'\n';_exit(77);}
            Fd lifetime(int(syscall(SYS_pidfd_open,getpid(),0)));assert(lifetime.n>=0);
            const auto init=fork();assert(init>=0);
            if(!init){bind_parent_lifetime(lifetime.n);const auto shell=fork();assert(shell>=0);if(!shell)target(ids[1],killed);close(ids[1]);_exit(supervise_target(shell));}
            close(ids[1]);int result=0;assert(waitpid(init,&result,0)==init);_exit(WIFEXITED(result)?WEXITSTATUS(result):128+WTERMSIG(result));
        }
        close(ids[1]);pid_t detached=0;ssize_t got=::read(ids[0],&detached,sizeof(detached));close(ids[0]);
        if(got!=sizeof(detached)){int result=0;waitpid(manager,&result,0);if(WIFEXITED(result)&&WEXITSTATUS(result)==77)return 77;assert(false);}
        if(killed)assert(kill(manager,SIGKILL)==0);
        int result=0;assert(waitpid(manager,&result,0)==manager);
        if(!killed)assert(WIFEXITED(result)&&WEXITSTATUS(result)==23);
        bool gone=false;
        for(unsigned i=0;i<200;++i){while(waitpid(-1,nullptr,WNOHANG)>0){}if(kill(detached,0)<0&&errno==ESRCH){gone=true;break;}std::this_thread::sleep_for(std::chrono::milliseconds(10));}
        assert(gone);
    }
    std::cout<<"Actual private PID namespace: shell status, detached job cleanup and manager SIGKILL lifetime passed with trusted native fixtures; no disks or target code\n";
}
