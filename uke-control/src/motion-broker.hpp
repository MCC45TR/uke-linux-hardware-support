// SPDX-License-Identifier: MIT
#pragma once
#include "motion-source.hpp"
#include <map>
#include <string>
#include <vector>
#include <systemd/sd-bus.h>
#include <sys/types.h>

namespace uke::motion {
struct Authorizer {
    using Ready=std::function<void(bool)>;
    virtual ~Authorizer()=default;
    virtual std::uint64_t request(sd_bus_message*,Ready)=0;
    virtual void cancel(std::uint64_t)=0;
    virtual bool active(uid_t)const=0;
};
std::unique_ptr<Authorizer> polkit_authorizer(sd_bus*);
class Broker {
public:
    using Factory=std::function<std::unique_ptr<Source>(std::function<void(Snapshot)>)>;
    Broker(sd_event*,sd_bus*,Authorizer&,Factory,bool exit_when_unused=true);
    ~Broker();
    Broker(const Broker&)=delete;Broker& operator=(const Broker&)=delete;
    void suspend(bool);
    void revalidate();
    void stop();
private:
    struct Pending {
        uid_t uid=0;
        std::uint64_t authorization=0;
        std::vector<sd_bus_message*> messages;
        ~Pending(){for(auto* m:messages)sd_bus_message_unref(m);}
    };
    sd_event* m_event;
    sd_bus* m_bus;
    Authorizer& m_authorizer;
    std::unique_ptr<Source> m_source;
    std::map<std::string,uid_t> m_leases;
    std::map<std::string,std::unique_ptr<Pending>> m_pending;
    sd_bus_slot *m_object=nullptr,*m_owners=nullptr,*m_sleep=nullptr;
    sd_event_source* m_idle=nullptr;
    bool m_suspended=false,m_stopping=false,m_exit_unused;
    void ready(const std::string&,bool);
    void publish(Snapshot);
    void release(const std::string&);
    void unused();
    static int claim(sd_bus_message*,void*,sd_bus_error*);
    static int relinquish(sd_bus_message*,void*,sd_bus_error*);
    static int owner_changed(sd_bus_message*,void*,sd_bus_error*);
    static int sleep_changed(sd_bus_message*,void*,sd_bus_error*);
    static int idle(sd_event_source*,std::uint64_t,void*);
};
}
