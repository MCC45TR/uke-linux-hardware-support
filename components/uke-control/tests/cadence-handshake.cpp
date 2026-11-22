// SPDX-License-Identifier: MIT
// A silent socket stands in for a stalled compositor. No real display is used.
#include "../src/cadence.cpp"
#include <chrono>
#include <sys/socket.h>
#include <sys/wait.h>
#include <thread>
int main() {
    for(int scenario=0;scenario<3;++scenario) {
        int sockets[2];assert(!socketpair(AF_UNIX,SOCK_STREAM,0,sockets));
        const auto started=std::chrono::steady_clock::now();const auto child=fork();assert(child>=0);
        if(!child) {
            close(sockets[1]);const auto fd=std::to_string(sockets[0]);assert(!setenv("WAYLAND_SOCKET",fd.c_str(),1));
            char name[]="cadence";char* args[]{name,nullptr};_exit(cadence::run(1,args)==0?1:0);
        }
        close(sockets[0]);
        if(scenario==1)close(sockets[1]);
        if(scenario==2){std::this_thread::sleep_for(std::chrono::milliseconds(100));assert(!kill(child,SIGINT));}
        int status=0;assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
        const auto elapsed=std::chrono::steady_clock::now()-started;
        assert(elapsed<std::chrono::seconds(scenario==0?7:2));
        if(scenario!=1)close(sockets[1]);
    }
    std::puts("PASS: silent, disconnected and cancelled Wayland registry handshakes");
}
