// SPDX-License-Identifier: MIT
#include "motion-broker.hpp"
#include <cassert>
#include <iostream>
#ifdef NDEBUG
#error Assertions are required
#endif
using namespace uke::motion;
namespace {
std::uint64_t now(){timespec ts{};assert(clock_gettime(CLOCK_MONOTONIC,&ts)==0);return std::uint64_t(ts.tv_sec)*1000000+std::uint64_t(ts.tv_nsec)/1000;}
struct Fixture {
    sd_event* event=nullptr;sd_bus *server=nullptr,*authority=nullptr,*client=nullptr;
    sd_bus_slot *object=nullptr,*policy=nullptr,*call=nullptr;
    sd_bus_message *incoming=nullptr,*held=nullptr;
    std::unique_ptr<Authorizer> auth;
    bool finished=false,allowed=false;unsigned requests=0;
    std::string mode="allow",expected_sender;
    Fixture(){
        assert(sd_event_new(&event)>=0);
        for(auto** bus:{&server,&authority,&client}){assert(sd_bus_open_user(bus)>=0);assert(sd_bus_attach_event(*bus,event,0)>=0);}
        static const sd_bus_vtable service[]={SD_BUS_VTABLE_START(0),SD_BUS_METHOD("Observe","","",observe,SD_BUS_VTABLE_UNPRIVILEGED),SD_BUS_VTABLE_END};
        static const sd_bus_vtable checker[]={SD_BUS_VTABLE_START(0),SD_BUS_METHOD("CheckAuthorization","(sa{sv})sa{ss}us","(bba{ss})",check,SD_BUS_VTABLE_UNPRIVILEGED),SD_BUS_VTABLE_END};
        assert(sd_bus_add_object_vtable(server,&object,"/test","org.uke.Test",service,this)>=0);
        assert(sd_bus_request_name(server,"org.uke.Test",0)>=0);
        assert(sd_bus_add_object_vtable(authority,&policy,"/org/freedesktop/PolicyKit1/Authority","org.freedesktop.PolicyKit1.Authority",checker,this)>=0);
        assert(sd_bus_request_name(authority,"org.freedesktop.PolicyKit1",0)>=0);
        const char* name=nullptr;assert(sd_bus_get_unique_name(client,&name)>=0);expected_sender=name;
        auth=polkit_authorizer(server);assert(!auth->active(0));
        assert(sd_bus_call_method_async(client,&call,"org.uke.Test","/test","org.uke.Test","Observe",ignored,nullptr,"")>=0);
        until([&]{return incoming!=nullptr;});
    }
    ~Fixture(){
        auth.reset();sd_bus_message_unref(incoming);sd_bus_message_unref(held);
        sd_bus_slot_unref(call);sd_bus_slot_unref(policy);sd_bus_slot_unref(object);
        for(auto* bus:{client,authority,server}){sd_bus_detach_event(bus);sd_bus_flush_close_unref(bus);}sd_event_unref(event);
    }
    void until(const std::function<bool()>& condition){const auto limit=now()+2500000;while(!condition()){assert(now()<limit);assert(sd_event_run(event,100000)>=0);}}
    std::uint64_t request(){return auth->request(incoming,[this](bool value){finished=true;allowed=value;});}
    static int ignored(sd_bus_message*,void*,sd_bus_error*){return 0;}
    static int observe(sd_bus_message* m,void* data,sd_bus_error*){auto& f=*static_cast<Fixture*>(data);f.incoming=sd_bus_message_ref(m);return 1;}
    static int check(sd_bus_message* m,void* data,sd_bus_error*){
        auto& f=*static_cast<Fixture*>(data);++f.requests;
        const char *subject=nullptr,*key=nullptr,*name=nullptr,*action=nullptr,*cancel=nullptr;unsigned flags=99;
        assert(sd_bus_message_enter_container(m,'r',"sa{sv}")>0);assert(sd_bus_message_read(m,"s",&subject)>0&&std::string_view(subject)=="system-bus-name");
        assert(sd_bus_message_enter_container(m,'a',"{sv}")>0);assert(sd_bus_message_enter_container(m,'e',"sv")>0);
        assert(sd_bus_message_read(m,"s",&key)>0&&std::string_view(key)=="name");assert(sd_bus_message_enter_container(m,'v',"s")>0);
        assert(sd_bus_message_read(m,"s",&name)>0&&name==f.expected_sender);
        for(unsigned i=0;i<4;++i)assert(sd_bus_message_exit_container(m)>0);
        assert(sd_bus_message_read(m,"s",&action)>0&&std::string_view(action)=="org.senemos.uke.motion.observe");
        assert(sd_bus_message_skip(m,"a{ss}")>0);assert(sd_bus_message_read(m,"us",&flags,&cancel)>0&&flags==0&&cancel&&!*cancel);
        if(f.mode=="hold"){f.held=sd_bus_message_ref(m);return 1;}
        if(f.mode=="malformed")return sd_bus_reply_method_return(m,"s","invalid");
        if(f.mode=="error")return sd_bus_reply_method_errorf(m,SD_BUS_ERROR_ACCESS_DENIED,"Fixture denial");
        return sd_bus_reply_method_return(m,"(bba{ss})",f.mode=="deny"?0:1,f.mode=="challenge"?1:0,0);
    }
};
}
int main(){
    for(auto mode:{"allow","deny","challenge","malformed","error","hold"}){
        Fixture f;f.mode=mode;assert(f.request()!=0);f.until([&]{return f.finished;});
        assert(f.requests==1&&f.allowed==(f.mode=="allow"));
    }
    {
        Fixture f;f.mode="hold";const auto id=f.request();assert(id);f.until([&]{return f.held!=nullptr;});
        f.auth->cancel(id);assert(sd_bus_reply_method_return(f.held,"(bba{ss})",1,0,0)>=0);
        // A second request gives a positive event-loop barrier after cancellation.
        f.mode="allow";assert(f.request());f.until([&]{return f.finished;});assert(f.allowed&&f.requests==2);
    }
    std::cout<<"Actual async Polkit encoding, unique-name subject, no-interaction flags, malformed/denied/challenge/timeout replies and cancellation passed; authority/session simulated\n";
}
