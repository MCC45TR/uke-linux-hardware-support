// SPDX-License-Identifier: MIT
// Readiness observer only; PID 1 owns the configured fastboot failure action.
#include <systemd/sd-login.h>
#include <wayland-client.h>
#include "boot-common.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/un.h>
#include <unistd.h>
namespace fs=std::filesystem;
static bool linux_root() {
    struct stat root{},partition{}; struct statfs filesystem{};
    return stat("/",&root)==0 && statfs("/",&filesystem)==0 && filesystem.f_type==0xef53 &&
        stat("/dev/disk/by-partlabel/linux",&partition)==0 && S_ISBLK(partition.st_mode) &&
        root.st_dev==partition.st_rdev;
}
static bool local_console_process(const std::string &comm,const std::string &terminal,
                                  const std::string &active,char state,long group,long foreground) {
    return (comm=="agetty" || comm=="login" || comm=="bash") &&
        terminal=="/dev/"+active && active.size()==4 && active.rfind("tty",0)==0 &&
        active[3]>='1' && active[3]<='6' && (state=='R'||state=='S') &&
        group>0 && group==foreground;
}
static bool local_console() {
    auto active=read_text("/sys/class/tty/tty0/active");
    if(!active.empty() && active.back()=='\n') active.pop_back();
    std::error_code error;
    for(const auto &entry:fs::directory_iterator("/proc",error)) {
        const auto name=entry.path().filename().string();
        if(name.empty() || name.find_first_not_of("0123456789")!=std::string::npos) continue;
        const auto info=read_text(entry.path()/"stat");
        const auto open=info.find('('),close=info.rfind(')');
        if(open==std::string::npos || close==std::string::npos || close<=open) continue;
        char state=0; long parent=0,group=0,session=0,tty=0,foreground=0;
        std::istringstream fields(info.substr(close+1));
        if(!(fields>>state>>parent>>group>>session>>tty>>foreground) || !tty) continue;
        auto terminal=fs::read_symlink(entry.path()/"fd/0",error).string();
        if(error) {error.clear(); continue;}
        if(local_console_process(info.substr(open+1,close-open-1),terminal,active,state,group,foreground)) return true;
    }
    return false;
}
static void sync_done(void *data,wl_callback *,uint32_t) { *static_cast<bool *>(data)=true; }
static bool wayland_ready(const fs::path &path,uid_t uid) {
    struct stat state{};
    if(lstat(path.c_str(),&state)!=0 || !S_ISSOCK(state.st_mode) || state.st_uid!=uid) return false;
    sockaddr_un address{}; address.sun_family=AF_UNIX;
    const auto name=path.string();
    if(name.size()>=sizeof(address.sun_path)) return false;
    std::memcpy(address.sun_path,name.c_str(),name.size()+1);
    const int fd=socket(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK,0);
    if(fd<0) return false;
    struct ucred peer{}; socklen_t length=sizeof(peer);
    if(connect(fd,reinterpret_cast<sockaddr *>(&address),sizeof(address))!=0 ||
       getsockopt(fd,SOL_SOCKET,SO_PEERCRED,&peer,&length)!=0 || peer.uid!=uid) {close(fd); return false;}
    auto *display=wl_display_connect_to_fd(fd);
    if(!display) {close(fd); return false;}
    bool done=false;
    auto *callback=wl_display_sync(display);
    static const wl_callback_listener listener{sync_done};
    if(!callback) {wl_display_disconnect(display); return false;}
    wl_callback_add_listener(callback,&listener,&done);
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(250);
    while(!done && std::chrono::steady_clock::now()<deadline) {
        if(wl_display_dispatch_pending(display)<0) break;
        if(done) break;
        if(wl_display_flush(display)<0 && errno!=EAGAIN) break;
        pollfd event{wl_display_get_fd(display),POLLIN,0};
        const int remaining=std::max(1,int(std::chrono::duration_cast<std::chrono::milliseconds>(deadline-std::chrono::steady_clock::now()).count()));
        const int result=poll(&event,1,remaining);
        if(result<=0 || (event.revents&(POLLERR|POLLHUP|POLLNVAL))) break;
        if((event.revents&POLLIN) && wl_display_dispatch(display)<0) break;
    }
    wl_callback_destroy(callback); wl_display_disconnect(display);
    return done;
}
static bool graphical_console() {
    char **sessions=nullptr;
    const int count=sd_get_sessions(&sessions);
    bool ready=false;
    for(int i=0; i<count; i++) {
        char *type=nullptr,*kind=nullptr; uid_t uid=0;
        if(!ready && sd_session_is_active(sessions[i])>0 &&
           sd_session_get_type(sessions[i],&type)>=0 && !std::strcmp(type,"wayland") &&
           sd_session_get_class(sessions[i],&kind)>=0 &&
           (!std::strcmp(kind,"user") || !std::strcmp(kind,"greeter")) &&
           sd_session_is_remote(sessions[i])==0 &&
           sd_session_get_uid(sessions[i],&uid)>=0 && uid!=0) {
            std::error_code error;
            const auto runtime=fs::path("/run/user")/std::to_string(uid);
            for(const auto &entry:fs::directory_iterator(runtime,error)) {
                const auto name=entry.path().filename().string();
                if(name.rfind("wayland-",0)==0 && name.size()<32 &&
                   name.find_first_not_of("0123456789",8)==std::string::npos &&
                   wayland_ready(entry.path(),uid)) {ready=true; break;}
            }
        }
        free(type); free(kind); free(sessions[i]);
    }
    free(sessions);
    return ready;
}
enum class Startup { wait,ready,timeout };
static Startup startup_state(double seconds,bool root,bool tty,bool graphics) {
    if(root && (tty||graphics)) return Startup::ready;
    return seconds>=180 ? Startup::timeout : Startup::wait;
}
static double boot_seconds() {
    timespec time{};
    if(clock_gettime(CLOCK_BOOTTIME,&time)!=0) throw std::runtime_error("Boot clock unavailable");
    return time.tv_sec+time.tv_nsec/1e9;
}
#ifndef UKE_BOOT_GUARD_TEST
int main(int argc,char **argv) {
    try {
        if(argc!=2 || (std::string(argv[1])!="--monitor" && std::string(argv[1])!="--check")) return 64;
        if(!uke_device(read_text("/sys/firmware/devicetree/base/compatible"))) {
            std::cout<<"UKE_BOOT_GUARD_INACTIVE: foreign device\n"; return 0;
        }
        for(;;) {
            const bool root=linux_root();
            const bool tty=root&&local_console();
            const bool graphics=root&&!tty&&graphical_console();
            const auto status=startup_state(boot_seconds(),root,tty,graphics);
            if(status==Startup::ready) {
                std::cout<<"UKE_BOOT_READY: "<<(tty?"local TTY":"responsive local Wayland session")<<'\n'; return 0;
            }
            if(std::string(argv[1])=="--check") return 2;
            if(status==Startup::timeout) {
                std::cerr<<"UKE_BOOT_TIMEOUT: no local console or responsive user session within 180 seconds; requesting fastboot\n";
                return 1;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch(const std::exception &) {
        std::cerr<<"UKE_BOOT_GUARD_ERROR: readiness inspection failed\n";
        return 1;
    }
}
#endif
