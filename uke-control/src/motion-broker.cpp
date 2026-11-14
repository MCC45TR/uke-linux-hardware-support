// SPDX-License-Identifier: MIT
#include "motion-broker.hpp"
#include <cerrno>
#include <cstdlib>
#include <stdexcept>
#include <systemd/sd-login.h>

namespace uke::motion {
namespace {
void require(int result,const char* message){if(result<0)throw std::runtime_error(message);}
class Polkit final:public Authorizer {
public:
    explicit Polkit(sd_bus* bus):m_bus(bus){}
    ~Polkit()override=default;
    bool active(uid_t uid)const override {
        if(uid==0)return false;
        char* raw=nullptr;if(sd_uid_get_display(uid,&raw)<0)return false;
        const std::unique_ptr<char,decltype(&std::free)> session(raw,std::free);
        if(!raw||!*raw)return false;
        uid_t owner=0;char* raw_seat=nullptr;
        const bool local=sd_session_get_uid(raw,&owner)>=0&&owner==uid&&sd_session_is_active(raw)>0&&
            sd_session_is_remote(raw)==0&&sd_session_get_seat(raw,&raw_seat)>=0&&raw_seat&&*raw_seat;
        std::free(raw_seat);return local;
    }
    std::uint64_t request(sd_bus_message* message,Ready ready)override {
        const char* sender=sd_bus_message_get_sender(message);if(!sender||sender[0]!=':')return 0;
        auto request=std::make_unique<Request>();request->owner=this;request->id=++m_next;request->ready=std::move(ready);
        sd_bus_message* raw=nullptr;
        require(sd_bus_message_new_method_call(m_bus,&raw,"org.freedesktop.PolicyKit1","/org/freedesktop/PolicyKit1/Authority",
            "org.freedesktop.PolicyKit1.Authority","CheckAuthorization"),"Cannot prepare authorization");
        const std::unique_ptr<sd_bus_message,decltype(&sd_bus_message_unref)> call(raw,sd_bus_message_unref);
        require(sd_bus_message_append(raw,"(sa{sv})sa{ss}us","system-bus-name",1,"name","s",sender,
            "org.senemos.uke.motion.observe",0,0U,""),"Cannot encode authorization");
        const auto id=request->id;
        require(sd_bus_call_async(m_bus,&request->slot,raw,complete,request.get(),1000000),"Cannot request authorization");
        m_requests.emplace(id,std::move(request));return id;
    }
    void cancel(std::uint64_t id)override {m_requests.erase(id);}
private:
    struct Request {
        Polkit* owner=nullptr;std::uint64_t id=0;Ready ready;sd_bus_slot* slot=nullptr;
        ~Request(){sd_bus_slot_unref(slot);}
    };
    sd_bus* m_bus;std::uint64_t m_next=0;
    std::map<std::uint64_t,std::unique_ptr<Request>> m_requests;
    static int complete(sd_bus_message* message,void* data,sd_bus_error*) {
        auto* pointer=static_cast<Request*>(data);auto& owner=*pointer->owner;
        auto node=owner.m_requests.extract(pointer->id);if(node.empty())return 0;
        auto request=std::move(node.mapped());int allowed=0,challenge=0;
        const bool accepted=!sd_bus_message_is_method_error(message,nullptr)&&
            sd_bus_message_has_signature(message,"(bba{ss})")&&
            sd_bus_message_enter_container(message,'r',"bba{ss}")>0&&
            sd_bus_message_read(message,"bb",&allowed,&challenge)>0&&allowed&&!challenge;
        request->ready(accepted);return 0;
    }
};
int reply(sd_bus_message* message,Snapshot snapshot) {
    return sd_bus_reply_method_return(message,"bdt",int(snapshot.known),snapshot.known?snapshot.inclination:0.0,
        snapshot.known?snapshot.timestamp_usec:std::uint64_t(0));
}
}
std::unique_ptr<Authorizer> polkit_authorizer(sd_bus* bus){return std::make_unique<Polkit>(bus);}
Broker::Broker(sd_event* event,sd_bus* bus,Authorizer& authorizer,Factory factory,bool exit_when_unused)
    :m_event(event),m_bus(bus),m_authorizer(authorizer),m_exit_unused(exit_when_unused) {
    static const sd_bus_vtable table[]={
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("Claim","","bdt",claim,SD_BUS_VTABLE_UNPRIVILEGED),
        SD_BUS_METHOD("Release","","",relinquish,SD_BUS_VTABLE_UNPRIVILEGED),
        SD_BUS_SIGNAL("PostureChanged","bdt",0),
        SD_BUS_VTABLE_END
    };
    try {
        m_source=factory([this](Snapshot value){publish(value);});
        if(!m_source)throw std::runtime_error("Missing motion source");
        require(sd_bus_add_object_vtable(bus,&m_object,object_path,interface,table,this),"Cannot export Motion1");
        require(sd_bus_match_signal(bus,&m_owners,"org.freedesktop.DBus","/org/freedesktop/DBus",
            "org.freedesktop.DBus","NameOwnerChanged",owner_changed,this),"Cannot watch leases");
        require(sd_bus_match_signal(bus,&m_sleep,"org.freedesktop.login1","/org/freedesktop/login1",
            "org.freedesktop.login1.Manager","PrepareForSleep",sleep_changed,this),"Cannot watch sleep");
        if(m_exit_unused)require(sd_event_add_time_relative(event,&m_idle,CLOCK_BOOTTIME,3000000,1000,idle,this),"Cannot bound idle activation");
    }catch(...){stop();sd_bus_slot_unref(m_object);sd_bus_slot_unref(m_owners);sd_bus_slot_unref(m_sleep);throw;}
}
Broker::~Broker(){stop();sd_bus_slot_unref(m_object);sd_bus_slot_unref(m_owners);sd_bus_slot_unref(m_sleep);sd_event_source_unref(m_idle);}
void Broker::stop() {
    if(m_stopping)return;
    m_stopping=true;
    for(const auto& [name,pending]:m_pending){
        (void)name;m_authorizer.cancel(pending->authorization);
        for(auto* request:pending->messages)sd_bus_reply_method_errorf(request,SD_BUS_ERROR_DISCONNECTED,"Motion service stopped");
    }
    m_pending.clear();
    if(m_source){m_source->stop();publish({});}
    m_leases.clear();
}
int Broker::claim(sd_bus_message* message,void* data,sd_bus_error* error) {
    auto& self=*static_cast<Broker*>(data);
    try {
        const char* raw=sd_bus_message_get_sender(message);
        if(self.m_stopping||!raw||raw[0]!=':')return sd_bus_error_set_const(error,SD_BUS_ERROR_ACCESS_DENIED,"Motion claim unavailable");
        const std::string sender(raw);
        if(const auto existing=self.m_leases.find(sender);existing!=self.m_leases.end()){
            if(!self.m_authorizer.active(existing->second)){self.release(sender);return sd_bus_error_set_const(error,SD_BUS_ERROR_ACCESS_DENIED,"Inactive local session");}
            return reply(message,self.m_suspended?Snapshot{}:self.m_source->current());
        }
        if(auto pending=self.m_pending.find(sender);pending!=self.m_pending.end()){
            if(pending->second->messages.size()>=8)return sd_bus_error_set_const(error,SD_BUS_ERROR_LIMITS_EXCEEDED,"Too many pending claims");
            pending->second->messages.push_back(sd_bus_message_ref(message));return 1;
        }
        if(self.m_leases.size()+self.m_pending.size()>=64)return sd_bus_error_set_const(error,SD_BUS_ERROR_LIMITS_EXCEEDED,"Motion lease limit reached");
        sd_bus_creds* raw_creds=nullptr;
        if(sd_bus_query_sender_creds(message,SD_BUS_CREDS_EUID,&raw_creds)<0)return sd_bus_error_set_const(error,SD_BUS_ERROR_ACCESS_DENIED,"Missing caller credentials");
        const std::unique_ptr<sd_bus_creds,decltype(&sd_bus_creds_unref)> creds(raw_creds,sd_bus_creds_unref);
        uid_t uid=0;
        if(sd_bus_creds_get_euid(raw_creds,&uid)<0||!self.m_authorizer.active(uid))return sd_bus_error_set_const(error,SD_BUS_ERROR_ACCESS_DENIED,"Only an active local session may observe posture");
        auto pending=std::make_unique<Pending>();pending->uid=uid;pending->messages.push_back(sd_bus_message_ref(message));
        auto* pointer=pending.get();self.m_pending.emplace(sender,std::move(pending));
        pointer->authorization=self.m_authorizer.request(message,[&self,sender](bool allowed){self.ready(sender,allowed);});
        if(!pointer->authorization)self.ready(sender,false);
        if(self.m_idle)sd_event_source_set_enabled(self.m_idle,SD_EVENT_OFF);
        return 1;
    }catch(...){
        const char* sender=sd_bus_message_get_sender(message);
        if(sender&&self.m_pending.contains(sender)){self.release(sender);return 1;}
        return sd_bus_error_set_const(error,SD_BUS_ERROR_FAILED,"Motion authorization failed");
    }
}
void Broker::ready(const std::string& sender,bool allowed) {
    auto node=m_pending.extract(sender);if(node.empty())return;
    auto pending=std::move(node.mapped());
    allowed=allowed&&!m_stopping&&m_authorizer.active(pending->uid);
    if(allowed){
        const bool first=m_leases.empty();m_leases.emplace(sender,pending->uid);
        if(first&&!m_suspended)m_source->start();
        const auto snapshot=m_suspended?Snapshot{}:m_source->current();
        for(auto* message:pending->messages)reply(message,snapshot);
    }else for(auto* message:pending->messages)sd_bus_reply_method_errorf(message,SD_BUS_ERROR_ACCESS_DENIED,"Motion observation requires an active local session");
    unused();
}
int Broker::relinquish(sd_bus_message* message,void* data,sd_bus_error*) {
    auto& self=*static_cast<Broker*>(data);const char* sender=sd_bus_message_get_sender(message);
    const int result=sd_bus_reply_method_return(message,"");
    if(sender)self.release(sender);
    return result;
}
void Broker::release(const std::string& sender) {
    bool removed_pending=false;
    if(auto node=m_pending.extract(sender);!node.empty()){
        removed_pending=true;
        auto pending=std::move(node.mapped());m_authorizer.cancel(pending->authorization);
        for(auto* message:pending->messages)sd_bus_reply_method_errorf(message,SD_BUS_ERROR_DISCONNECTED,"Motion claim released");
    }
    const bool removed_lease=m_leases.erase(sender)>0;
    if(removed_lease&&m_leases.empty())m_source->stop();
    // Other system-bus clients may disappear between activation and Claim.
    // They do not own a lease and must not end this service's startup grace.
    if(removed_pending||removed_lease)unused();
}
void Broker::unused() {
    if(m_pending.empty()&&m_leases.empty()&&m_exit_unused){
        sd_bus_flush(m_bus);sd_event_exit(m_event,0);
    }
}
void Broker::publish(Snapshot snapshot) {
    if(m_suspended||m_stopping)snapshot={};
    if(snapshot.known&&(!std::isfinite(snapshot.inclination)||snapshot.inclination<0||snapshot.inclination>180||!snapshot.timestamp_usec))snapshot={};
    for(const auto& [name,uid]:m_leases){
        (void)uid;sd_bus_message* raw=nullptr;
        if(sd_bus_message_new_signal(m_bus,&raw,object_path,interface,"PostureChanged")<0)continue;
        const std::unique_ptr<sd_bus_message,decltype(&sd_bus_message_unref)> signal(raw,sd_bus_message_unref);
        if(sd_bus_message_set_destination(raw,name.c_str())<0)continue;
        if(sd_bus_message_append(raw,"bdt",int(snapshot.known),snapshot.known?snapshot.inclination:0.0,
            snapshot.known?snapshot.timestamp_usec:std::uint64_t(0))<0)continue;
        sd_bus_send(m_bus,raw,nullptr);
    }
}
int Broker::owner_changed(sd_bus_message* message,void* data,sd_bus_error*) {
    const char *name=nullptr,*old_owner=nullptr,*new_owner=nullptr;
    if(sd_bus_message_read(message,"sss",&name,&old_owner,&new_owner)>0&&name&&name[0]==':'&&old_owner&&*old_owner&&new_owner&&!*new_owner)
        static_cast<Broker*>(data)->release(name);
    return 0;
}
void Broker::suspend(bool sleeping) {
    if(m_suspended==sleeping)return;
    m_suspended=sleeping;
    if(sleeping){m_source->stop();publish({});}
    else {revalidate();if(!m_leases.empty())m_source->start();}
}
int Broker::sleep_changed(sd_bus_message* message,void* data,sd_bus_error*) {
    int sleeping=0;if(sd_bus_message_read(message,"b",&sleeping)>0)static_cast<Broker*>(data)->suspend(sleeping);
    return 0;
}
void Broker::revalidate() {
    std::vector<std::string> revoked;
    for(const auto& [name,uid]:m_leases)if(!m_authorizer.active(uid))revoked.push_back(name);
    // Revoke visibility before releasing each targeted lease.
    for(const auto& name:revoked){
        sd_bus_message* signal=nullptr;
        if(sd_bus_message_new_signal(m_bus,&signal,object_path,interface,"PostureChanged")>=0){
            sd_bus_message_set_destination(signal,name.c_str());sd_bus_message_append(signal,"bdt",0,0.0,std::uint64_t(0));
            sd_bus_send(m_bus,signal,nullptr);sd_bus_message_unref(signal);
        }
        release(name);
    }
}
int Broker::idle(sd_event_source*,std::uint64_t,void* data){static_cast<Broker*>(data)->unused();return 0;}
}
