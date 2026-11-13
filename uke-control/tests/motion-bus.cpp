// SPDX-License-Identifier: MIT
#include "motion-broker.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <systemd/sd-event.h>
#ifdef NDEBUG
#error Assertions are required
#endif
using namespace uke::motion;
namespace {
std::uint64_t clock_now() {
    timespec ts{};assert(clock_gettime(CLOCK_MONOTONIC,&ts)==0);
    return std::uint64_t(ts.tv_sec)*1000000+std::uint64_t(ts.tv_nsec)/1000;
}
struct Auth:Authorizer {
    bool eligible=true,throw_request=false;std::uint64_t next=0;unsigned cancelled=0;
    std::map<std::uint64_t,Ready> waiting;
    bool active(uid_t)const override{return eligible;}
    std::uint64_t request(sd_bus_message*,Ready ready)override{if(throw_request)throw std::runtime_error("Fixture request failure");waiting.emplace(++next,std::move(ready));return next;}
    void cancel(std::uint64_t id)override{cancelled+=waiting.erase(id);}
    void finish(bool allowed){assert(waiting.size()==1);auto node=waiting.extract(waiting.begin());node.mapped()(allowed);}
};
struct Sensor:Source {
    std::function<void(Snapshot)> notify;
    bool running=false;unsigned starts=0,stops=0;Snapshot value{};
    bool start()override {if(!running){running=true;++starts;}return true;}
    void stop()override{if(running){running=false;++stops;}value={};}
    Snapshot current()const override{return running?value:Snapshot{};}
    void emit(Snapshot snapshot){assert(running);value=snapshot;notify(snapshot);}
};
struct Answer {
    bool done=false,accepted=false;Snapshot snapshot{};sd_bus_slot* slot=nullptr;
    ~Answer(){sd_bus_slot_unref(slot);}
    static int received(sd_bus_message* message,void* data,sd_bus_error*) {
        auto& self=*static_cast<Answer*>(data);self.done=true;
        self.accepted=!sd_bus_message_is_method_error(message,nullptr);
        if(self.accepted&&sd_bus_message_has_signature(message,"bdt")){
            int known=0;assert(sd_bus_message_read(message,"bdt",&known,&self.snapshot.inclination,&self.snapshot.timestamp_usec)>0);self.snapshot.known=known;
        }
        return 0;
    }
};
struct Client {
    sd_bus* bus=nullptr;sd_bus_slot* slot=nullptr;unsigned signals=0;Snapshot last{};
    explicit Client(sd_event* event){
        assert(sd_bus_open_user(&bus)>=0);assert(sd_bus_attach_event(bus,event,0)>=0);
        assert(sd_bus_match_signal(bus,&slot,bus_name,object_path,interface,"PostureChanged",changed,this)>=0);
    }
    ~Client(){close();}
    void close(){sd_bus_slot_unref(slot);slot=nullptr;if(bus)sd_bus_detach_event(bus);sd_bus_flush_close_unref(bus);bus=nullptr;}
    void call(const char* member,Answer& answer){
        assert(sd_bus_call_method_async(bus,&answer.slot,bus_name,object_path,interface,member,Answer::received,&answer,"")>=0);
    }
    void barrier(Answer& answer){
        assert(sd_bus_call_method_async(bus,&answer.slot,bus_name,object_path,"org.freedesktop.DBus.Peer","Ping",Answer::received,&answer,"")>=0);
    }
    static int changed(sd_bus_message* message,void* data,sd_bus_error*) {
        auto& self=*static_cast<Client*>(data);assert(sd_bus_message_has_signature(message,"bdt"));int known=0;
        assert(sd_bus_message_read(message,"bdt",&known,&self.last.inclination,&self.last.timestamp_usec)>0);self.last.known=known;++self.signals;return 0;
    }
};
struct Fixture {
    sd_event* event=nullptr;sd_bus* bus=nullptr;Auth auth;Sensor* sensor=nullptr;std::unique_ptr<Broker> broker;
    Fixture(){
        assert(sd_event_new(&event)>=0);assert(sd_bus_open_user(&bus)>=0);assert(sd_bus_attach_event(bus,event,0)>=0);
        broker=std::make_unique<Broker>(event,bus,auth,[this](auto notify){
            auto source=std::make_unique<Sensor>();source->notify=std::move(notify);sensor=source.get();return source;
        },false);
        assert(sd_bus_request_name(bus,bus_name,0)>=0);
    }
    ~Fixture(){broker.reset();sd_bus_detach_event(bus);sd_bus_flush_close_unref(bus);sd_event_unref(event);}
    void until(const std::function<bool()>& condition){
        const auto limit=clock_now()+2000000;
        while(!condition()){assert(clock_now()<limit);assert(sd_event_run(event,100000)>=0);}
    }
    void claim(Client& client,Answer& answer){
        client.call("Claim",answer);until([&]{return !auth.waiting.empty();});auth.finish(true);until([&]{return answer.done;});assert(answer.accepted);
    }
    void release(Client& client){Answer answer;client.call("Release",answer);until([&]{return answer.done;});assert(answer.accepted);}
};
void authorization() {
    Fixture f;Client a(f.event);
    f.auth.eligible=false;Answer inactive;a.call("Claim",inactive);f.until([&]{return inactive.done;});assert(!inactive.accepted&&!f.sensor->starts&&f.auth.waiting.empty());
    f.auth.eligible=true;Answer denied;a.call("Claim",denied);f.until([&]{return !f.auth.waiting.empty();});f.auth.finish(false);f.until([&]{return denied.done;});assert(!denied.accepted&&!f.sensor->starts);
    f.auth.throw_request=true;Answer unavailable;a.call("Claim",unavailable);f.until([&]{return unavailable.done;});assert(!unavailable.accepted&&f.auth.waiting.empty()&&!f.sensor->starts);f.auth.throw_request=false;
    Answer pending;a.call("Claim",pending);f.until([&]{return !f.auth.waiting.empty();});f.release(a);f.until([&]{return pending.done;});assert(!pending.accepted&&f.auth.cancelled==1&&f.auth.waiting.empty()&&!f.sensor->starts);
    Answer lost;
    {Client temporary(f.event);temporary.call("Claim",lost);f.until([&]{return !f.auth.waiting.empty();});}
    f.until([&]{return f.auth.waiting.empty();});assert(f.auth.cancelled==2&&!f.sensor->starts);
    Answer vanished;a.call("Claim",vanished);f.until([&]{return !f.auth.waiting.empty();});f.auth.eligible=false;f.auth.finish(true);f.until([&]{return vanished.done;});assert(!vanished.accepted&&!f.sensor->starts);
}
void leases() {
    Fixture f;Client a(f.event),b(f.event),observer(f.event);
    Answer first,duplicate;a.call("Claim",first);a.call("Claim",duplicate);f.until([&]{return !f.auth.waiting.empty();});
    // Drain the second method before the delayed authorization completes.
    Answer barrier;a.call("Release",barrier);f.until([&]{return barrier.done&&first.done&&duplicate.done;});
    assert(!first.accepted&&!duplicate.accepted&&f.sensor->starts==0);
    Answer accepted;f.claim(a,accepted);assert(!accepted.snapshot.known&&f.sensor->starts==1);
    f.sensor->emit({true,12.0,1234});f.until([&]{return a.signals==1;});assert(a.last.known&&a.last.timestamp_usec==1234);
    Answer repeated;a.call("Claim",repeated);f.until([&]{return repeated.done;});assert(repeated.accepted&&repeated.snapshot.known&&repeated.snapshot.inclination==12&&f.auth.waiting.empty()&&f.sensor->starts==1);
    Answer second;f.claim(b,second);assert(second.snapshot.known&&f.sensor->starts==1);
    f.sensor->emit({true,46,2345});f.until([&]{return a.signals==2&&b.signals==1;});assert(observer.signals==0);
    f.release(a);f.release(a);assert(f.sensor->running&&f.sensor->stops==0);
    f.sensor->emit({});f.until([&]{return b.signals==2;});assert(!b.last.known&&b.last.timestamp_usec==0&&observer.signals==0);
    b.close();f.until([&]{return !f.sensor->running;});assert(f.sensor->stops==1);
}
void repeated_pending() {
    Fixture f;Client a(f.event);Answer first,duplicate,ordered;
    a.call("Claim",first);a.call("Claim",duplicate);a.barrier(ordered);
    f.until([&]{return ordered.done;});assert(ordered.accepted&&f.auth.waiting.size()==1&&!first.done&&!duplicate.done);
    f.auth.finish(true);f.until([&]{return first.done&&duplicate.done;});
    assert(first.accepted&&duplicate.accepted&&f.sensor->starts==1);
    f.release(a);assert(f.sensor->stops==1);
}
void sleep_and_session() {
    Fixture f;Client a(f.event),observer(f.event);Answer answer;f.claim(a,answer);
    f.sensor->emit({true,0,4567});f.until([&]{return a.signals==1;});
    f.broker->suspend(true);f.until([&]{return a.signals==2;});assert(!a.last.known&&a.last.timestamp_usec==0&&!f.sensor->running&&f.sensor->stops==1);
    Answer sleeping;a.call("Claim",sleeping);f.until([&]{return sleeping.done;});assert(sleeping.accepted&&!sleeping.snapshot.known&&f.sensor->starts==1);
    f.broker->suspend(false);assert(f.sensor->running&&f.sensor->starts==2);
    f.sensor->emit({true,180,5678});f.until([&]{return a.signals==3;});assert(a.last.inclination==180);
    f.auth.eligible=false;f.broker->revalidate();f.until([&]{return a.signals==4;});assert(!a.last.known&&!f.sensor->running&&f.sensor->stops==2&&observer.signals==0);
    f.broker->suspend(true);f.broker->suspend(false);assert(f.sensor->starts==2);
    f.auth.eligible=true;Answer pending;a.call("Claim",pending);f.until([&]{return !f.auth.waiting.empty();});f.broker->stop();f.until([&]{return pending.done;});assert(!pending.accepted&&f.auth.waiting.empty()&&f.auth.cancelled==1);
}
}
int main(){authorization();leases();repeated_pending();sleep_and_session();std::cout<<"Actual Motion1 broker: async authorization, idempotent/targeted leases, release/owner loss, suspend and session revocation passed; simulated source only\n";}
