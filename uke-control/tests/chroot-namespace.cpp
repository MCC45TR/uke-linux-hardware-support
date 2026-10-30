// SPDX-License-Identifier: MIT
// Real private tmpfs lifecycle only; no block devices or target code.
#include "../src/chroot.cpp"
#include <cassert>
using namespace uke::chroot_manager;
int main(){
    for(bool cancel:{false,true}) {
        int ready[2],finish[2];assert(pipe(ready)==0&&pipe(finish)==0);
        const auto child=fork();assert(child>=0);
        if(!child){close(ready[0]);close(finish[1]);signals();try{
            Fd before(::open("/proc/self/mountinfo",O_RDONLY|O_CLOEXEC));const auto baseline=read_fd(before.n,1024*1024);
            Mounts mounts;assert(lseek(before.n,0,SEEK_SET)==0);assert(read_fd(before.n,1024*1024)==baseline);
            assert(file("/proc/self/mountinfo",1024*1024)!=baseline);std::ofstream(mounts.root/"fixture")<<"private";
            assert(::write(ready[1],"x",1)==1);char byte;while(::read(finish[0],&byte,1)<0&&errno==EINTR){if(interrupted)break;}
            need(!interrupted,"interrupted");assert(mounts.cleanup());assert(file("/proc/self/mountinfo",1024*1024)==baseline);_exit(0);
        }catch(const std::exception& error){std::cerr<<error.what()<<'\n';_exit(interrupted?130:77);}}
        close(ready[1]);close(finish[0]);char byte;assert(::read(ready[0],&byte,1)==1);close(ready[0]);
        assert(!fs::exists("/run/uke-chroot/root/fixture"));
        if(cancel)kill(child,SIGTERM);else assert(::write(finish[1],"x",1)==1);close(finish[1]);
        int status=0;assert(waitpid(child,&status,0)==child);assert(WIFEXITED(status)&&WEXITSTATUS(status)==(cancel?130:0));
        assert(!fs::exists("/run/uke-chroot/root/fixture"));
    }
    std::cout<<"Actual mount namespace: private runtime/tmpfs visibility, normal cleanup and SIGTERM cleanup passed; no block device or target code\n";
}
