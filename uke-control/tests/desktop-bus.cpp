// SPDX-License-Identifier: MIT
// Real wire messages on a private bus; never a desktop or hardware test.
#include "../src/desktop.hpp"
#include <systemd/sd-json.h>
#include <atomic>
#include <array>
#include <cerrno>
#include <cassert>
#include <cmath>
#include <cstring>
#include <memory>
#include <thread>
static std::atomic<bool> invalid_percentage = false;
static std::atomic<int> screensaver_state = 0;
static std::atomic<bool> night_supported = true;
static int get_color(sd_bus *,const char *,const char *,const char *name,sd_bus_message *reply,void *,sd_bus_error *) {
    if(!std::strcmp(name,"Temperature"))return sd_bus_message_append(reply,"u",night_supported?4200U:0U);
    return sd_bus_message_append(reply,"b",!std::strcmp(name,"DisabledUntilTomorrow")?0:int(night_supported));
}
static int get(sd_bus *, const char *, const char *, const char *name, sd_bus_message *reply, void *, sd_bus_error *) {
    if (!std::strcmp(name,"State")) return sd_bus_message_append(reply,"u",2U);
    if (!std::strcmp(name,"Percentage")) return sd_bus_message_append(reply,"d",invalid_percentage ? NAN : 3.0);
    if (!std::strcmp(name,"EnergyRate")) return sd_bus_message_append(reply,"d",4.0);
    if (!std::strcmp(name,"TimeToFull")) return sd_bus_message_append(reply,"x",int64_t(0));
    if (!std::strcmp(name,"TimeToEmpty")) return sd_bus_message_append(reply,"x",int64_t(1080));
    if (!std::strcmp(name,"OnBattery")) return sd_bus_message_append(reply,"b",0);
    if (!std::strcmp(name,"LidIsClosed") || !std::strcmp(name,"PreparingForSleep")) return sd_bus_message_append(reply,"b",0);
    if (!std::strcmp(name,"LidIsPresent") || !std::strcmp(name,"LidClosed") || !std::strcmp(name,"Docked")) return sd_bus_message_append(reply,"b",1);
    if (!std::strcmp(name,"DaemonVersion")) return sd_bus_message_append(reply,"s","fixture");
    if (!std::strcmp(name,"ChargeThresholdSupported")) return sd_bus_message_append(reply,"b",0);
    if (!std::strcmp(name,"ChargeEndThreshold")) return sd_bus_message_append(reply,"u",0U);
    if (!std::strcmp(name,"Temperature")) return sd_bus_message_append(reply,"d",-5.0);
    if (!std::strcmp(name,"ActiveProfile")) return sd_bus_message_append(reply,"b",1); // Deliberately wrong type.
    return -EINVAL;
}
static int method(sd_bus_message *m,void *,sd_bus_error *) {
    const char *name=sd_bus_message_get_member(m);
    if (!std::strcmp(name,"EnumerateDevices")) return sd_bus_reply_method_return(m,"ao",1,"/org/freedesktop/UPower/devices/battery_BAT0");
    if (!std::strcmp(name,"GetCriticalAction")) return sd_bus_reply_method_return(m,"s","PowerOff");
    if (!std::strcmp(name,"CanSuspend")) return sd_bus_reply_method_return(m,"s","yes");
    if (!std::strcmp(name,"CanHibernate")) return sd_bus_reply_method_return(m,"s","no");
    if (!std::strcmp(name,"GetActive")) {
        if (screensaver_state < 0) return sd_bus_reply_method_errorf(m,"org.freedesktop.DBus.Error.NotSupported","Fixture unavailable");
        return sd_bus_reply_method_return(m,"b",int(screensaver_state));
    }
    assert(!std::strcmp(name,"batteryRemainingTime"));
    return sd_bus_reply_method_return(m,"t",uint64_t(1080000));
}
static int filter(sd_bus_message *m,void *,sd_bus_error *) {
    uint8_t type=0;assert(sd_bus_message_get_type(m,&type)>=0);
    if(type==SD_BUS_MESSAGE_METHOD_CALL) {
            assert(sd_bus_message_get_auto_start(m)==0);
            const char *member=sd_bus_message_get_member(m);
            assert(!std::strcmp(member,"GetAll") || !std::strcmp(member,"GetCriticalAction") || !std::strcmp(member,"EnumerateDevices") ||
                !std::strcmp(member,"CanSuspend") || !std::strcmp(member,"CanHibernate") || !std::strcmp(member,"batteryRemainingTime") || !std::strcmp(member,"GetActive"));
    }
    return 0;
}
static void verify(const std::string &json,bool bad) {
    sd_json_variant *raw=nullptr;
    assert(sd_json_parse(json.c_str(),sd_json_parse_flags_t(0),&raw,nullptr,nullptr)>=0);
    std::unique_ptr<sd_json_variant,decltype(&sd_json_variant_unref)> root(raw,sd_json_variant_unref);
    assert(!sd_json_variant_boolean(sd_json_variant_by_key(raw,"physical_acceptance")));
    auto list=sd_json_variant_by_key(raw,"interfaces");assert(sd_json_variant_elements(list)==13);
    auto display=sd_json_variant_by_index(list,1);
    assert(sd_json_variant_boolean(sd_json_variant_by_key(display,"available")));
    auto properties=sd_json_variant_by_key(display,"properties");
    auto percentage=sd_json_variant_by_key(sd_json_variant_by_key(properties,"Percentage"),"value");
    assert(bad?sd_json_variant_is_null(percentage):sd_json_variant_real(percentage)==3);
    auto full=sd_json_variant_by_key(properties,"TimeToFull");
    assert(sd_json_variant_is_null(sd_json_variant_by_key(full,"value")));
    auto empty=sd_json_variant_by_key(properties,"TimeToEmpty");
    assert(sd_json_variant_real(sd_json_variant_by_key(empty,"value"))==1080);
    assert(!std::strcmp(sd_json_variant_string(sd_json_variant_by_key(empty,"unit")),"s"));
    auto pack=sd_json_variant_by_key(sd_json_variant_by_index(list,2),"properties");
    assert(sd_json_variant_is_null(sd_json_variant_by_key(sd_json_variant_by_key(pack,"ChargeEndThreshold"),"value")));
    assert(sd_json_variant_real(sd_json_variant_by_key(sd_json_variant_by_key(pack,"Temperature"),"value"))==-5);
    auto ppd=sd_json_variant_by_key(sd_json_variant_by_index(list,4),"properties");
    assert(sd_json_variant_is_null(sd_json_variant_by_key(sd_json_variant_by_key(ppd,"ActiveProfile"),"value")));
    assert(!sd_json_variant_boolean(sd_json_variant_by_key(sd_json_variant_by_index(list,5),"available")));
    auto kde=sd_json_variant_by_index(list,8);
    assert(sd_json_variant_unsigned(sd_json_variant_by_key(kde,"value"))==1080000);
    assert(!std::strcmp(sd_json_variant_string(sd_json_variant_by_key(kde,"unit")),"ms"));
    auto upower=sd_json_variant_by_key(sd_json_variant_by_index(list,0),"properties");
    auto logind=sd_json_variant_by_key(sd_json_variant_by_index(list,9),"properties");
    auto upower_lid=sd_json_variant_by_key(sd_json_variant_by_key(upower,"LidIsClosed"),"value");
    auto logind_lid=sd_json_variant_by_key(sd_json_variant_by_key(logind,"LidClosed"),"value");
    assert(sd_json_variant_is_boolean(upower_lid) && !sd_json_variant_boolean(upower_lid));
    assert(sd_json_variant_is_boolean(logind_lid) && sd_json_variant_boolean(logind_lid));
    auto saver=sd_json_variant_by_index(list,10);
    auto saver_value=sd_json_variant_by_key(saver,"value");
    assert(sd_json_variant_boolean(sd_json_variant_by_key(saver,"available"))==(screensaver_state>=0));
    if(screensaver_state<0)assert(sd_json_variant_is_null(saver_value));
    else assert(sd_json_variant_is_boolean(saver_value) && sd_json_variant_boolean(saver_value)==bool(screensaver_state));
    auto mutter=sd_json_variant_by_key(sd_json_variant_by_index(list,11),"properties");
    auto supported=sd_json_variant_by_key(sd_json_variant_by_key(mutter,"NightLightSupported"),"value");
    assert(sd_json_variant_is_boolean(supported) && sd_json_variant_boolean(supported)==night_supported);
    auto color=sd_json_variant_by_key(sd_json_variant_by_index(list,12),"properties");
    auto temperature=sd_json_variant_by_key(sd_json_variant_by_key(color,"Temperature"),"value");
    assert(night_supported?sd_json_variant_unsigned(temperature)==4200:sd_json_variant_is_null(temperature));
}
int main() {
    sd_bus *server=nullptr,*client=nullptr;std::array<sd_bus_slot*,9> slots{};
    assert(sd_bus_open_user(&server)>=0 && sd_bus_open_user(&client)>=0);
    static const sd_bus_vtable root[]={SD_BUS_VTABLE_START(0),
        SD_BUS_PROPERTY("DaemonVersion","s",get,0,0), SD_BUS_PROPERTY("OnBattery","b",get,0,0),
        SD_BUS_PROPERTY("LidIsClosed","b",get,0,0), SD_BUS_PROPERTY("LidIsPresent","b",get,0,0),
        SD_BUS_METHOD("EnumerateDevices","","ao",method,SD_BUS_VTABLE_UNPRIVILEGED),
        SD_BUS_METHOD("GetCriticalAction","","s",method,SD_BUS_VTABLE_UNPRIVILEGED),SD_BUS_VTABLE_END};
    static const sd_bus_vtable battery[]={SD_BUS_VTABLE_START(0),
        SD_BUS_PROPERTY("State","u",get,0,0), SD_BUS_PROPERTY("Percentage","d",get,0,0),
        SD_BUS_PROPERTY("EnergyRate","d",get,0,0), SD_BUS_PROPERTY("TimeToFull","x",get,0,0),
        SD_BUS_PROPERTY("TimeToEmpty","x",get,0,0),SD_BUS_VTABLE_END};
    static const sd_bus_vtable login[]={SD_BUS_VTABLE_START(0),
        SD_BUS_PROPERTY("LidClosed","b",get,0,0), SD_BUS_PROPERTY("Docked","b",get,0,0),
        SD_BUS_PROPERTY("PreparingForSleep","b",get,0,0),
        SD_BUS_METHOD("CanSuspend","","s",method,SD_BUS_VTABLE_UNPRIVILEGED),
        SD_BUS_METHOD("CanHibernate","","s",method,SD_BUS_VTABLE_UNPRIVILEGED),SD_BUS_VTABLE_END};
    static const sd_bus_vtable kde[]={SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("batteryRemainingTime","","t",method,SD_BUS_VTABLE_UNPRIVILEGED),SD_BUS_VTABLE_END};
    static const sd_bus_vtable pack[]={SD_BUS_VTABLE_START(0),
        SD_BUS_PROPERTY("Temperature","d",get,0,0),
        SD_BUS_PROPERTY("ChargeThresholdSupported","b",get,0,0),
        SD_BUS_PROPERTY("ChargeEndThreshold","u",get,0,0),SD_BUS_VTABLE_END};
    static const sd_bus_vtable ppd[]={SD_BUS_VTABLE_START(0),
        SD_BUS_PROPERTY("ActiveProfile","b",get,0,0),SD_BUS_VTABLE_END};
    static const sd_bus_vtable saver[]={SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("GetActive","","b",method,SD_BUS_VTABLE_UNPRIVILEGED),SD_BUS_VTABLE_END};
    static const sd_bus_vtable mutter[]={SD_BUS_VTABLE_START(0),
        SD_BUS_PROPERTY("NightLightSupported","b",get_color,0,0),SD_BUS_VTABLE_END};
    static const sd_bus_vtable color[]={SD_BUS_VTABLE_START(0),
        SD_BUS_PROPERTY("NightLightActive","b",get_color,0,0),
        SD_BUS_PROPERTY("Temperature","u",get_color,0,0),
        SD_BUS_PROPERTY("DisabledUntilTomorrow","b",get_color,0,0),SD_BUS_VTABLE_END};
    assert(sd_bus_add_object_vtable(server,&slots[0],"/org/freedesktop/UPower","org.freedesktop.UPower",root,nullptr)>=0);
    assert(sd_bus_add_object_vtable(server,&slots[1],"/org/freedesktop/UPower/devices/DisplayDevice","org.freedesktop.UPower.Device",battery,nullptr)>=0);
    assert(sd_bus_add_object_vtable(server,&slots[2],"/org/freedesktop/login1","org.freedesktop.login1.Manager",login,nullptr)>=0);
    assert(sd_bus_add_object_vtable(server,&slots[3],"/org/kde/Solid/PowerManagement","org.kde.Solid.PowerManagement",kde,nullptr)>=0);
    assert(sd_bus_add_object_vtable(server,&slots[4],"/org/freedesktop/UPower/devices/battery_BAT0","org.freedesktop.UPower.Device",pack,nullptr)>=0);
    assert(sd_bus_add_object_vtable(server,&slots[5],"/org/freedesktop/UPower/PowerProfiles","org.freedesktop.UPower.PowerProfiles",ppd,nullptr)>=0);
    assert(sd_bus_add_object_vtable(server,&slots[6],"/ScreenSaver","org.freedesktop.ScreenSaver",saver,nullptr)>=0);
    assert(sd_bus_add_object_vtable(server,&slots[7],"/org/gnome/Mutter/DisplayConfig","org.gnome.Mutter.DisplayConfig",mutter,nullptr)>=0);
    assert(sd_bus_add_object_vtable(server,&slots[8],"/org/gnome/SettingsDaemon/Color","org.gnome.SettingsDaemon.Color",color,nullptr)>=0);
    assert(sd_bus_add_filter(server,nullptr,filter,nullptr)>=0);
    for(const auto *name:{"org.freedesktop.UPower","org.freedesktop.UPower.PowerProfiles","org.freedesktop.login1","org.kde.Solid.PowerManagement","org.freedesktop.ScreenSaver","org.gnome.Mutter.DisplayConfig","org.gnome.SettingsDaemon.Color"})
        assert(sd_bus_request_name(server,name,0)>=0);
    std::atomic<bool> done=false;
    std::thread worker([&]{while(!done){int r=sd_bus_process(server,nullptr);assert(r>=0);if(!r)assert(sd_bus_wait(server,10000)>=0);}});
    verify(uke::desktop::snapshot(client,client),false);
    invalid_percentage=true;screensaver_state=1;verify(uke::desktop::snapshot(client,client),true);
    screensaver_state=-1;verify(uke::desktop::snapshot(client,client),true);
    night_supported=false;verify(uke::desktop::snapshot(client,client),true);
    auto missing=uke::desktop::snapshot(nullptr,nullptr);assert(missing.find("\"available\":true")==std::string::npos);
    done=true;worker.join();for(auto *slot:slots)sd_bus_slot_unref(slot);sd_bus_unref(client);sd_bus_unref(server);
}
