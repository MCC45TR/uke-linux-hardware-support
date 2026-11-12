// SPDX-License-Identifier: MIT
#include "motion-broker.hpp"
#include <csignal>
#include <iostream>
#include <systemd/sd-daemon.h>
#include <systemd/sd-login.h>
#include <unistd.h>

namespace {
int session_changed(sd_event_source*,int,std::uint32_t,void* data) {
    auto* pair=static_cast<std::pair<sd_login_monitor*,uke::motion::Broker*>*>(data);
    sd_login_monitor_flush(pair->first);pair->second->revalidate();return 0;
}
int terminate(sd_event_source*,const signalfd_siginfo*,void* data) {
    auto* pair=static_cast<std::pair<sd_event*,uke::motion::Broker*>*>(data);
    pair->second->stop();return sd_event_exit(pair->first,0);
}
void need(int result,const char* message){if(result<0)throw std::runtime_error(message);}
}
int main(int argc,char** argv) {
    if(argc==2&&std::string_view(argv[1])=="--recover")return uke::motion::recover_iio()?0:1;
    if(argc!=1){std::cerr<<"Usage: motion [--recover]\n";return 2;}
    sd_event* event=nullptr;sd_bus* bus=nullptr;sd_login_monitor* login=nullptr;
    sd_event_source *login_source=nullptr,*term_source=nullptr,*int_source=nullptr;
    int result=1;
    try {
        std::uint64_t watchdog=0;
        if(geteuid()!=0||sd_watchdog_enabled(0,&watchdog)<=0||watchdog>2000000)
            throw std::runtime_error("A bounded systemd watchdog is required");
        sigset_t blocked;sigemptyset(&blocked);sigaddset(&blocked,SIGTERM);sigaddset(&blocked,SIGINT);
        if(sigprocmask(SIG_BLOCK,&blocked,nullptr)<0)throw std::runtime_error("Cannot block termination signals");
        if(!uke::motion::recover_iio())throw std::runtime_error("Motion ownership recovery refused");
        need(sd_event_new(&event),"Cannot create event loop");
        need(sd_bus_open_system(&bus),"Cannot open system bus");
        need(sd_bus_set_method_call_timeout(bus,500000),"Cannot bound bus calls");
        need(sd_bus_attach_event(bus,event,0),"Cannot attach system bus");
        auto authorizer=uke::motion::polkit_authorizer(bus);
        uke::motion::Broker broker(event,bus,*authorizer,[event](auto notify){return uke::motion::iio_source(event,std::move(notify));});
        need(sd_login_monitor_new("session",&login),"Cannot monitor active sessions");
        std::pair<sd_login_monitor*,uke::motion::Broker*> login_state{login,&broker};
        need(sd_event_add_io(event,&login_source,sd_login_monitor_get_fd(login),sd_login_monitor_get_events(login),session_changed,&login_state),"Cannot watch session changes");
        std::pair<sd_event*,uke::motion::Broker*> signal_state{event,&broker};
        need(sd_event_add_signal(event,&term_source,SIGTERM,terminate,&signal_state),"Cannot handle termination");
        need(sd_event_add_signal(event,&int_source,SIGINT,terminate,&signal_state),"Cannot handle interruption");
        need(sd_bus_request_name(bus,uke::motion::bus_name,0),"Cannot own Motion1");
        need(sd_event_set_watchdog(event,1),"Cannot establish event-loop watchdog");
        sd_notify(0,"READY=1");result=sd_event_loop(event)<0?1:0;broker.stop();
        // Remove callbacks before their stack contexts leave scope.
        login_source=sd_event_source_unref(login_source);term_source=sd_event_source_unref(term_source);int_source=sd_event_source_unref(int_source);
        sd_bus_flush(bus);
    }catch(const std::exception&){std::cerr<<"Motion service unavailable; posture remains unknown\n";}
    sd_event_source_unref(login_source);sd_event_source_unref(term_source);sd_event_source_unref(int_source);
    sd_login_monitor_unref(login);if(bus)sd_bus_detach_event(bus);sd_bus_flush_close_unref(bus);sd_event_unref(event);return result;
}
