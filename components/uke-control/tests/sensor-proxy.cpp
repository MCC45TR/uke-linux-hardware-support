// SPDX-License-Identifier: MIT
// Private-bus interface fixture: no tablet, service activation or sensor device.
#include "../src/sensors.cpp"
#include <cassert>
#include <poll.h>
#include <sys/prctl.h>
#include <sys/wait.h>
namespace {
int events=-1;
int method(sd_bus_message* m,void*,sd_bus_error*) {
    assert(sd_bus_message_get_auto_start(m)==0);
    const char event=std::string_view(sd_bus_message_get_member(m)).starts_with("Claim")?'C':'R';
    assert(write(events,&event,1)==1);return sd_bus_reply_method_return(m,"");
}
int property(sd_bus*,const char*,const char*,const char* name,sd_bus_message* reply,void*,sd_bus_error*) {
    const std::string n=name;
    if(n.starts_with("Has"))return sd_bus_message_append(reply,"b",1);
    if(n=="ProximityNear")return sd_bus_message_append(reply,"b",0);
    if(n=="LightLevel")return sd_bus_message_append(reply,"d",123.5);
    if(n=="CompassHeading")return sd_bus_message_append(reply,"d",17.0);
    return sd_bus_message_append(reply,"s",n=="LightLevelUnit"?"lux":n=="AccelerometerOrientation"?"left-up":"vertical");
}
#define PROP(name,type) SD_BUS_PROPERTY(name,type,property,0,SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE)
#define METHOD(name) SD_BUS_METHOD(name,"","",method,SD_BUS_VTABLE_UNPRIVILEGED)
const sd_bus_vtable main_table[]={SD_BUS_VTABLE_START(0),
    METHOD("ClaimAccelerometer"),METHOD("ReleaseAccelerometer"),METHOD("ClaimLight"),METHOD("ReleaseLight"),
    METHOD("ClaimProximity"),METHOD("ReleaseProximity"),PROP("HasAccelerometer","b"),PROP("HasAmbientLight","b"),
    PROP("HasProximity","b"),PROP("ProximityNear","b"),PROP("LightLevel","d"),PROP("LightLevelUnit","s"),
    PROP("AccelerometerOrientation","s"),PROP("AccelerometerTilt","s"),SD_BUS_VTABLE_END};
const sd_bus_vtable compass_table[]={SD_BUS_VTABLE_START(0),METHOD("ClaimCompass"),METHOD("ReleaseCompass"),
    PROP("HasCompass","b"),PROP("CompassHeading","d"),SD_BUS_VTABLE_END};
struct Server {
    pid_t pid=-1;int input=-1;
    Server() {
        int pipefd[2];assert(pipe2(pipefd,O_CLOEXEC)==0);pid=fork();assert(pid>=0);
        if(pid==0) {
            prctl(PR_SET_PDEATHSIG,SIGKILL);if(getppid()==1)_exit(1);
            close(pipefd[0]);events=pipefd[1];sd_bus* bus=nullptr;assert(sd_bus_open_system(&bus)>=0);
            assert(sd_bus_add_object_vtable(bus,nullptr,"/net/hadess/SensorProxy","net.hadess.SensorProxy",main_table,nullptr)>=0);
            assert(sd_bus_add_object_vtable(bus,nullptr,"/net/hadess/SensorProxy/Compass","net.hadess.SensorProxy.Compass",compass_table,nullptr)>=0);
            assert(sd_bus_request_name(bus,"net.hadess.SensorProxy",0)>=0);assert(write(events,"!",1)==1);
            for(;;){const int r=sd_bus_process(bus,nullptr);assert(r>=0);if(r==0)sd_bus_wait(bus,1'000'000);}
        }
        close(pipefd[1]);input=pipefd[0];pollfd f{input,POLLIN,0};assert(poll(&f,1,3000)==1);
        char ready=0;assert(::read(input,&ready,1)==1&&ready=='!');fcntl(input,F_SETFL,O_NONBLOCK);
    }
    void stop(){if(pid>0){kill(pid,SIGTERM);int status;assert(waitpid(pid,&status,0)==pid);pid=-1;}}
    std::string collected(){std::string s;char b[128];ssize_t n;while((n=::read(input,b,sizeof(b)))>0)s.append(b,size_t(n));return s;}
    ~Server(){stop();if(input>=0)close(input);}
};
}
int main() {
    using namespace uke::sensors;Server first;
    {
        Proxy p(true);std::vector<Reading> rows;p.append(rows,false);
        bool light=false,near=false,compass=false;
        for(const auto& r:rows){if(r.label=="LightLevel"){assert(r.value==123.5&&r.unit=="lux");light=true;}if(r.label=="ProximityNear"){assert(r.text=="false");near=true;}if(r.label=="CompassHeading"){assert(r.value==17);compass=true;}}
        assert(light&&near&&compass);
    }
    assert(first.collected()=="CCCCRRRR");
    {Proxy list(false);std::vector<Reading> rows;list.append(rows,true);assert(rows.size()==4);}
    assert(first.collected().empty());
    {
        Proxy original(true);assert(first.collected()=="CCCC");first.stop();Server replacement;
        std::vector<Reading> rows;original.append(rows,false);
        for(const auto& r:rows)assert(!r.value&&r.text.empty());
        assert(replacement.collected().empty());
        {Proxy reopened(true);rows.clear();reopened.append(rows,false);}
        assert(replacement.collected()=="CCCCRRRR");
        original.reconnect();rows.clear();original.append(rows,false);
        assert(replacement.collected()=="CCCC");
        bool recovered=false;for(const auto& row:rows)if(row.label=="LightLevel"&&row.value==123.5)recovered=true;
        assert(recovered);
    }
    std::cout<<"PASS: SensorProxy claims, units, releases, no activation, owner pinning and explicit fresh-owner reconnection\n";
}
