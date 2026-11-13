// SPDX-License-Identifier: MIT
#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <string>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <systemd/sd-daemon.h>
#include <systemd/sd-event.h>
#include <unistd.h>
#ifdef NDEBUG
#error Assertions are required
#endif
int main(){
    char pattern[]="/tmp/uke-motion-watchdog-XXXXXX";const char* directory=mkdtemp(pattern);assert(directory);
    const std::string path=std::string(directory)+"/notify";
    const int listener=socket(AF_UNIX,SOCK_DGRAM|SOCK_CLOEXEC|SOCK_NONBLOCK,0);assert(listener>=0);
    sockaddr_un address{};address.sun_family=AF_UNIX;assert(path.size()<sizeof(address.sun_path));std::strcpy(address.sun_path,path.c_str());
    assert(bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0);
    const auto parent=getpid(),child=fork();assert(child>=0);
    if(child==0){
        assert(prctl(PR_SET_PDEATHSIG,SIGKILL)==0);if(getppid()!=parent)_exit(127);close(listener);
        assert(setenv("NOTIFY_SOCKET",path.c_str(),1)==0);assert(setenv("WATCHDOG_USEC","1000000",1)==0);
        const auto pid=std::to_string(getpid());assert(setenv("WATCHDOG_PID",pid.c_str(),1)==0);
        std::uint64_t interval=0;assert(sd_watchdog_enabled(0,&interval)>0&&interval==1000000);
        sd_event* event=nullptr;assert(sd_event_new(&event)>=0);assert(sd_event_set_watchdog(event,1)>=0);
        _exit(sd_event_loop(event)<0?1:0);
    }
    auto heartbeat=[&]{
        pollfd fd{listener,POLLIN,0};int result=0;
        do{result=poll(&fd,1,3000);}while(result<0&&errno==EINTR);
        assert(result==1);char buffer[256]{};const auto bytes=recv(listener,buffer,sizeof(buffer),0);
        assert(bytes>0&&std::string_view(buffer,std::size_t(bytes)).find("WATCHDOG=1")!=std::string_view::npos);
    };
    heartbeat();heartbeat();
    assert(kill(child,SIGSTOP)==0);int status=0;assert(waitpid(child,&status,WUNTRACED)==child&&WIFSTOPPED(status));
    char buffer[256];while(recv(listener,buffer,sizeof(buffer),0)>0){}assert(errno==EAGAIN||errno==EWOULDBLOCK);
    pollfd fd{listener,POLLIN,0};assert(poll(&fd,1,300)==0);
    assert(kill(child,SIGKILL)==0);assert(waitpid(child,&status,0)==child&&WIFSIGNALED(status)&&WTERMSIG(status)==SIGKILL);
    close(listener);assert(unlink(path.c_str())==0&&rmdir(directory)==0);
    std::cout<<"Actual sd-event watchdog heartbeat stops with a frozen event loop; supervisor kill simulated, systemd unit acceptance remains open\n";
}
