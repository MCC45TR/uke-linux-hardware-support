// SPDX-License-Identifier: MIT
#define UKE_BOOT_GUARD_TEST
#include "../src/boot-guard.cpp"
#include <wayland-server.h>
#include <cassert>
#include <csignal>
#include <sys/wait.h>
int main() {
    assert(uke_device(std::string("xiaomi,uke\0qcom,sm7675\0",23)));
    assert(!uke_device("xiaomi,uke"));
    assert(!uke_device(std::string("xiaomi,nabu\0qcom,sm8150\0",24)));
    assert(startup_state(179.9,false,true,true)==Startup::wait);
    assert(startup_state(180,false,true,true)==Startup::timeout);
    assert(startup_state(180,true,false,false)==Startup::timeout);
    assert(startup_state(179,true,true,false)==Startup::ready);
    assert(startup_state(179,true,false,true)==Startup::ready);
    assert(local_console_process("bash","/dev/tty1","tty1",'S',55,55));
    assert(local_console_process("agetty","/dev/tty2","tty2",'R',55,55));
    assert(!local_console_process("bash","/dev/tty1","tty2",'S',55,55));
    assert(!local_console_process("bash","/dev/ttyGS0","ttyGS0",'S',55,55));
    assert(!local_console_process("bash","/dev/tty1","tty1",'D',55,55));
    assert(!local_console_process("bash","/dev/tty1","tty1",'S',55,56));
    assert(!local_console_process("bash","/dev/pts/0","tty1",'S',55,55));
    assert(!local_console_process("systemd","/dev/tty1","tty1",'S',55,55));
    assert(boot_seconds()>0);
    char directory[]="/tmp/uke-wayland-test.XXXXXX";
    assert(mkdtemp(directory));
    assert(setenv("XDG_RUNTIME_DIR",directory,1)==0);
    auto *server=wl_display_create(); assert(server);
    const char *socket_name=wl_display_add_socket_auto(server); assert(socket_name);
    const auto path=fs::path(directory)/socket_name;
    const pid_t child=fork(); assert(child>=0);
    if(child==0) {wl_display_run(server); _exit(0);}
    assert(wayland_ready(path,getuid()));
    assert(!wayland_ready(path,getuid()+1));
    assert(kill(child,SIGTERM)==0); assert(waitpid(child,nullptr,0)==child);
    wl_display_destroy(server);
    const int silent=socket(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0); assert(silent>=0);
    sockaddr_un address{}; address.sun_family=AF_UNIX;
    const auto stalled=fs::path(directory)/"wayland-99";
    strcpy(address.sun_path,stalled.c_str());
    assert(bind(silent,reinterpret_cast<sockaddr *>(&address),sizeof(address))==0);
    assert(listen(silent,1)==0);
    const auto start=std::chrono::steady_clock::now();
    assert(!wayland_ready(stalled,getuid()));
    assert(std::chrono::steady_clock::now()-start<std::chrono::seconds(2));
    close(silent);
    std::ofstream(stalled.string()+"-regular")<<"not a Wayland socket";
    assert(!wayland_ready(stalled.string()+"-regular",getuid()));
    fs::remove_all(directory);
    std::cout<<"Boot deadline, foreground TTY, responsive/stalled Wayland and device identity tests passed\n";
}
