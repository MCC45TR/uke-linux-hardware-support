// SPDX-License-Identifier: MIT
#include "../src/critical-health.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <initializer_list>
#include <unistd.h>

static void put(const std::filesystem::path &path,const std::string &text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);out<<text;assert(out.good());
}
int main() {
    char temp[]="/tmp/uke-critical-health-XXXXXX";
    const auto *name=mkdtemp(temp);assert(name);
    const std::filesystem::path root=name,adsp=root/"sys/class/remoteproc/remoteproc7";
    unsigned cases=0;
    const auto unavailable=[&] {
        try{critical_platform_health(root);assert(false);}
        catch(const CriticalHealthFailure &){assert(false);}
        catch(const std::runtime_error &){}
        cases++;
    };
    unavailable();
    for(bool waiting:{false,true}) {
        assert(critical_state_event("userdata.state",waiting));cases++;
        assert(critical_state_event("rollback.conf",waiting));cases++;
        assert(critical_state_event("lock",waiting)==waiting);cases++;
        assert(!critical_state_event("critical-health",waiting));cases++;
    }
    assert(critical_remoteproc_event("remoteproc","adsp","crash"));cases++;
    for(const auto &fields:std::initializer_list<std::initializer_list<std::string>>{
            {"remoteproc","cdsp","crash"},{"devcoredump","adsp","crash"},
            {"remoteproc","adsp","running"},{"remoteproc","adsp",""}}) {
        auto p=fields.begin();const auto &subsystem=*p++;const auto &name=*p++;const auto &event=*p;
        assert(!critical_remoteproc_event(subsystem,name,event));cases++;
    }
    put(adsp/"name","cdsp\n");put(adsp/"state","crashed\n");unavailable();
    put(adsp/"name","adsp\n");put(adsp/"state","running\n");critical_platform_health(root);cases++;
    put(adsp/"state","crashed\n");
    try{critical_platform_health(root);assert(false);}catch(const CriticalHealthFailure &e){assert(std::string(e.what()).find("ADSP")!=std::string::npos);}cases++;
    {
        bool latched=false;
        try{critical_platform_health_latched(latched,root);assert(false);}
        catch(const CriticalHealthFailure &){assert(latched);}cases++;
        put(adsp/"state","running\n");
        try{critical_platform_health_latched(latched,root);assert(false);}
        catch(const CriticalHealthFailure &){assert(latched);}cases++;
    }
    for(const auto &value:std::initializer_list<std::string>{"offline\n","suspended\n","recovering\n","running\ncrashed\n",std::string(129,'x')+"\n"}) {
        put(adsp/"state",value);
        try{critical_platform_health(root);assert(false);}
        catch(const CriticalHealthFailure &){assert(false);}
        catch(const std::runtime_error &){}cases++;
    }
    for(const auto &mode:{"enforced","permissive","off"})for(bool critical:{false,true}) {
        assert(critical_fallback_policy(mode,critical)==(std::string(mode)=="enforced" || (std::string(mode)=="permissive" && critical)));cases++;
    }
    const std::string release="7.3.3-9-unstable",boot="01234567-89ab-cdef-0123-456789abcdef",hash(64,'a');
    const std::string marker="format=1\nkernel="+release+"\nboot="+boot+"\nstate_sha256="+hash+"\ncritical=adsp-crashed\n";
    assert(critical_rejection_matches(marker,release,boot,hash));cases++;
    assert(!critical_rejection_matches(marker,"7.3.3-7-unstable",boot,hash));cases++;
    assert(!critical_rejection_matches(marker,release,"11234567-89ab-cdef-0123-456789abcdef",hash));cases++;
    assert(!critical_rejection_matches(marker,release,boot,std::string(64,'b')));cases++;
    assert(!critical_rejection_matches(marker+"extra=1\n",release,boot,hash));cases++;
    assert(!critical_rejection_matches(marker.substr(0,marker.size()-1),release,boot,hash));cases++;
    assert(!critical_rejection_matches(marker,release,boot,std::string(64,'z')));cases++;
    for(const std::string mode:{"enforced","permissive","off"})for(bool busy:{false,true}) {
        unsigned writes=0,logs=0;bool off_reported=false,threw=false;
        const auto event=[&] {
            critical_runtime_event([]{throw CriticalHealthFailure("critical: ADSP crashed");},
                [&]{return mode;},[&]{writes++;return !busy;},
                [&](const auto &observed,const auto &,bool authority,bool critical){assert(observed==mode && authority==(mode!="off") && critical);logs++;},off_reported);
        };
        try{event();}catch(const CriticalHealthFailure &){threw=true;}
        assert(threw==(mode!="off" && !busy));
        assert(writes==(mode=="off"?0u:1u));
        assert(logs==(mode=="off" || !busy?1u:0u));
        if(mode=="off"){event();assert(logs==1 && !writes);}
        cases++;
    }
    {
        bool reported=false;unsigned writes=0;
        critical_runtime_event([]{throw std::runtime_error("ADSP recovering");},[]{return "permissive";},
            [&]{writes++;return true;},[](const auto &,const auto &,bool,bool){assert(false);},reported);
        assert(!writes && !reported);cases++;
    }
    put(adsp/"state","running\n");
    assert(!critical_crash_count(adsp/"crash_count"));cases++;
    put(adsp/"crash_count","0\n");
    assert(critical_crash_count(adsp/"crash_count")==0);critical_platform_health(root);cases++;
    for(const std::string value:{"1\n","12\n","4294967295\n"}) {
        put(adsp/"crash_count",value);
        bool latched=false;
        try{critical_platform_health_latched(latched,root);assert(false);}
        catch(const CriticalHealthFailure &){assert(latched);}cases++;
        put(adsp/"crash_count","0\n");
        try{critical_platform_health_latched(latched,root);assert(false);}
        catch(const CriticalHealthFailure &){assert(latched);}cases++;
    }
    for(const std::string &value:std::initializer_list<std::string>{"","-1\n","+1\n","1.0\n","00\n"," 1\n","1 \n",
            "4294967296\n","1","1\r\n","1\n2\n","0\n\n",std::string(128,'9')+"\n",std::string("1\0\n",3)}) {
        put(adsp/"crash_count",value);unavailable();
        put(adsp/"state","crashed\n");
        try{critical_platform_health(root);assert(false);}
        catch(const CriticalHealthFailure &){}cases++;
        put(adsp/"state","running\n");
        for(const std::string mode:{"enforced","permissive","off"}) {
            unsigned writes=0,logs=0;bool off=false,unknown=false;
            const auto probe=[&] {
                critical_runtime_event([&]{critical_platform_health(root);},[&]{return mode;},
                    [&]{writes++;return true;},
                    [&](const auto &seen,const auto &,bool authority,bool critical){assert(seen==mode && !authority && !critical);logs++;},
                    off,&unknown);
            };
            probe();probe();assert(logs==1 && !writes && unknown && !off);
            put(adsp/"crash_count","0\n");probe();assert(!unknown && logs==1);
            put(adsp/"crash_count",value);probe();assert(logs==2 && !writes);
            cases++;
        }
    }
    std::filesystem::remove(adsp/"crash_count");
    critical_platform_health(root);cases++; // Older kernel retains state/event support.
    std::filesystem::remove_all(root);
    std::cout<<"Actual critical health: "<<cases<<" sysfs/policy/boot-state binding scenarios passed; no device writer or reboot provider.\n";
}
