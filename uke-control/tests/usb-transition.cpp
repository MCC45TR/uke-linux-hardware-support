// SPDX-License-Identifier: MIT
// Root in a disposable user/mount namespace; no USB or driver nodes.
#include "usb-transition.hpp"
#include <cassert>
#include <signal.h>
#include <sys/wait.h>
namespace fs=std::filesystem;
using uke::usb::TransitionInhibit;
template<class F>void refused(F&& call){bool denied=false;try{call();}catch(const std::exception&){denied=true;}assert(denied);}
int main() {
    if(geteuid()!=0)return 77;
    char pattern[]="/tmp/uke-usb-transition-XXXXXX";assert(mkdtemp(pattern));const fs::path root=pattern,path=root/"inhibit";
    assert(TransitionInhibit::status(path)=="absent");
    {TransitionInhibit inhibit(path);assert(TransitionInhibit::status(path)=="active");refused([&]{TransitionInhibit second(path);});bool checked=false;refused([&]{TransitionInhibit::recover([&]{checked=true;return true;},path);});assert(!checked);}
    assert(!fs::exists(path));
    for(bool changed:{false,true}) {
        int ready[2];assert(!pipe(ready));const auto child=fork();assert(child>=0);
        if(!child){close(ready[0]);TransitionInhibit inhibit(path);if(changed)inhibit.begin_write();assert(write(ready[1],"R",1)==1);for(;;)pause();}
        close(ready[1]);char byte=0;assert(read(ready[0],&byte,1)==1&&byte=='R');close(ready[0]);assert(!kill(child,SIGKILL));int status=0;assert(waitpid(child,&status,0)==child);
        if(!changed) {
            assert(TransitionInhibit::status(path)=="reserved_stale");
            // An unrelated live PID is not ownership proof; flock remains authoritative.
            std::ifstream old(path);std::string data((std::istreambuf_iterator<char>(old)),{});old.close();
            const auto at=data.find("pid=");const auto end=data.find('\n',at);data.replace(at,end-at,"pid="+std::to_string(getpid()));std::ofstream(path)<<data;
            {TransitionInhibit next(path);}assert(!fs::exists(path));
        } else {
            assert(TransitionInhibit::status(path)=="needs_recovery");refused([&]{TransitionInhibit next(path);});
            refused([&]{TransitionInhibit::recover([]{return false;},path);});assert(fs::exists(path));
            TransitionInhibit::recover([]{return true;},path);assert(!fs::exists(path));
        }
    }
    {TransitionInhibit inhibit(path);inhibit.begin_write();}
    assert(TransitionInhibit::status(path)=="needs_recovery");TransitionInhibit::recover([]{return true;},path);
    {TransitionInhibit inhibit(path);inhibit.begin_write();inhibit.complete();}assert(!fs::exists(path));
    {TransitionInhibit inhibit(path);fs::remove(path);std::ofstream(path)<<"foreign";}assert(fs::exists(path));fs::remove(path);
    std::ofstream(path).close();assert(!chmod(path.c_str(),0600));refused([&]{TransitionInhibit inhibit(path);});refused([&]{TransitionInhibit::recover([]{return true;},path);});assert(fs::exists(path));fs::remove(path);
    std::ofstream(root/"foreign")<<"foreign";fs::create_symlink(root/"foreign",path);refused([&]{TransitionInhibit inhibit(path);});fs::remove(path);
    assert(fs::exists(root/"foreign"));fs::remove_all(root);
}
