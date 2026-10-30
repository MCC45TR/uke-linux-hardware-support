// SPDX-License-Identifier: MIT
// Wait/shutdown only: no helper, desktop notification or hardware operation.
#include "../src/charge-notify.cpp"
#include <cassert>
#include <iostream>
#include <thread>
namespace uke::charge { Result control_query(const std::string&) { throw std::runtime_error("Unexpected helper execution"); } }
static bool blocked(int signal) {
    sigset_t mask;assert(pthread_sigmask(SIG_SETMASK,nullptr,&mask)==0);
    return sigismember(&mask,signal)==1;
}
int main() {
    using namespace uke::charge;
    signal(SIGTERM,stop);signal(SIGINT,stop);
    const bool term_blocked=blocked(SIGTERM),int_blocked=blocked(SIGINT);
    const auto begin=std::chrono::steady_clock::now();wait_interval();
    assert(std::chrono::steady_clock::now()-begin>=std::chrono::milliseconds(1900));
    for(const auto event:{SIGTERM,SIGINT}) {
        stopping=0;const auto target=pthread_self();
        const auto start=std::chrono::steady_clock::now();
        std::thread sender([=]{std::this_thread::sleep_for(std::chrono::milliseconds(50));assert(pthread_kill(target,event)==0);});
        wait_interval();sender.join();
        const auto elapsed=std::chrono::steady_clock::now()-start;
        assert(stopping&&elapsed<std::chrono::milliseconds(1500));
        assert(blocked(SIGTERM)==term_blocked&&blocked(SIGINT)==int_blocked);
    }
    const auto stopped=std::chrono::steady_clock::now();wait_interval();
    assert(std::chrono::steady_clock::now()-stopped<std::chrono::milliseconds(500));
    assert(blocked(SIGTERM)==term_blocked&&blocked(SIGINT)==int_blocked);
    std::cout<<"Notification wait: monotonic interval, SIGTERM/SIGINT interruption, stopped state and mask restoration passed\n";
}
