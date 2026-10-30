// SPDX-License-Identifier: MIT
#include "rgb.hpp"
#include "cli.hpp"
#include "lights.hpp"
#include <charconv>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <systemd/sd-bus.h>
#include <unistd.h>
namespace uke::rgb {
static void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Color parse_hex(std::string_view text) {
    if(text.starts_with('#'))text.remove_prefix(1);
    need(text.size()==6,"RGB color must be #RRGGBB");
    for(char c:text)need((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'),"Invalid HEX color");
    unsigned value=0;auto result=std::from_chars(text.data(),text.data()+text.size(),value,16);
    need(result.ec==std::errc{} && result.ptr==text.data()+text.size(),"Invalid HEX color");
    return {value>>16,(value>>8)&255,value&255};
}
std::string hex(Color color) {
    for(auto n:color)need(n<=255,"RGB component exceeds 255");
    char value[8];std::snprintf(value,sizeof(value),"#%02x%02x%02x",color[0],color[1],color[2]);return value;
}
int command(int argc,char** argv,bool as_json) {
    const bool status=argc==1 && std::string_view(argv[0])=="status";
    const bool test=argc==1 && std::string_view(argv[0])=="test";
    const bool off=argc==1 && (std::string_view(argv[0])=="off" || std::string_view(argv[0])=="auto");
    Color color{};unsigned percent=0;
    if(!status&&!test&&!off) {
        need(argc>=2 && std::string_view(argv[0])=="set","Usage: uke-control led status|auto|off|test|set HEX [BRIGHTNESS]");
        if(argc==2 || argc==3) {color=parse_hex(argv[1]);percent=argc==3?lights::percent(argv[2]):100;}
        else if(argc==4 || argc==5) {
            for(unsigned i=0;i<3;++i)color[i]=lights::color(argv[i+1]);
            percent=argc==5?lights::percent(argv[4]):100;
        } else throw std::runtime_error("Usage: uke-control led set #RRGGBB [0..100]");
    }
    if(!status)need(geteuid()!=0,"Run RGB requests as the active desktop user; sudo is not needed");
    sd_bus* raw=nullptr;need(sd_bus_open_system(&raw)>=0,"System bus unavailable");
    std::unique_ptr<sd_bus,decltype(&sd_bus_unref)> bus(raw,sd_bus_unref);
    sd_bus_set_method_call_timeout(raw,3000000);
    sd_bus_message* answer=nullptr;sd_bus_error error=SD_BUS_ERROR_NULL;
    const char* method=status?"Status":test?"TestColors":off?"Automatic":"SetColor";
    const int result=status||test||off?
        sd_bus_call_method(raw,bus_name,object_path,interface,method,&error,&answer,""):
        sd_bus_call_method(raw,bus_name,object_path,interface,method,&error,&answer,"su",hex(color).c_str(),percent);
    std::unique_ptr<sd_bus_message,decltype(&sd_bus_message_unref)> reply(answer,sd_bus_message_unref);
    const std::string message=error.message?error.message:"RGB service unavailable";sd_bus_error_free(&error);
    if(result<0) {
        if(status){uke::cli::print("{\"schema\":\"org.uke.control.rgb.v1\",\"available\":false,\"mode\":null,\"reason\":\"service_unavailable\"}\n",as_json,"Notification LED");return 0;}
        throw std::runtime_error(message);
    }
    const char* text=nullptr;need(sd_bus_message_read(answer,"s",&text)>0&&text,"Invalid RGB service response");
    uke::cli::print(text,as_json,"Notification LED");return 0;
}
}
