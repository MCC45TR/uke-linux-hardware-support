// SPDX-License-Identifier: MIT
#include "../src/nfc.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
namespace fs=std::filesystem;
void put(const fs::path& p,const std::string& value){fs::create_directories(p.parent_path());std::ofstream f(p);f<<value;assert(f.good());}
template<class F>void refuse(F f){bool bad=false;try{f();}catch(const std::exception&){bad=true;}assert(bad);}
int main(){
    using namespace uke::nfc;char pattern[]="/tmp/uke-nfc-fixture-XXXXXX";auto made=mkdtemp(pattern);assert(made);fs::path sys=made;
    try{
        assert(radios(sys).empty());refuse([&]{request(radios(sys),true);});
        auto nfc=sys/"devices/usb/nfc/nfc0",rf=nfc/"rfkill/rfkill4";fs::create_directories(nfc);
        put(rf/"type","nfc\n");put(rf/"soft","1\n");put(rf/"hard","0\n");
        fs::create_directories(sys/"class/nfc");fs::create_directories(sys/"class/rfkill");
        fs::create_directory_symlink(nfc,sys/"class/nfc/nfc0");fs::create_directory_symlink(rf,sys/"class/rfkill/rfkill4");
        auto found=radios(sys);assert(found.size()==1&&found[0].index==4);
        auto tag=sys/"devices/i2c/1-0057",node=sys/"firmware/devicetree/base/i2c/tag";
        put(node/"compatible","fm,fm19511\n");fs::create_directories(tag);fs::create_directory_symlink(node,tag/"of_node");
        fs::create_directories(sys/"bus/i2c/devices");fs::create_directory_symlink(tag,sys/"bus/i2c/devices/1-0057");
        assert(snapshot(sys).find("\"compatible\":\"fm,fm19511\"")!=std::string::npos);
        auto event=request(found,true);assert(event.idx==4&&event.type==RFKILL_TYPE_NFC&&event.op==RFKILL_OP_CHANGE&&event.soft==0);
        assert(request(found,false).soft==1);
        auto text=snapshot(sys);assert(text.find("\"soft_blocked\":true")!=text.npos&&text.find("\"powered\":null")!=text.npos&&text.find(made)==text.npos);
        put(rf/"hard","1\n");refuse([&]{request(radios(sys),true);});assert(request(radios(sys),false).soft==1);
        put(rf/"hard","bad\n");refuse([&]{request(radios(sys),false);});put(rf/"hard","0\n");
        auto second=nfc/"rfkill/rfkill5";put(second/"type","nfc\n");put(second/"soft","0\n");put(second/"hard","0\n");fs::create_directory_symlink(second,sys/"class/rfkill/rfkill5");
        refuse([&]{request(radios(sys),true);});fs::remove(sys/"class/rfkill/rfkill5");
        auto bluetooth=sys/"devices/usb/bluetooth/rfkill4";put(bluetooth/"type","bluetooth\n");put(bluetooth/"soft","0\n");put(bluetooth/"hard","0\n");
        fs::remove(sys/"class/rfkill/rfkill4");fs::create_directory_symlink(bluetooth,sys/"class/rfkill/rfkill4");refuse([&]{request(radios(sys),false);});
        fs::remove(sys/"class/rfkill/rfkill4");fs::create_directory_symlink(rf,sys/"class/rfkill/rfkill4");
        fs::create_directory_symlink(nfc,sys/"class/nfc/nfc1");refuse([&]{request(radios(sys),true);});
        fs::remove(sys/"class/nfc/nfc1");fs::remove(sys/"class/nfc/nfc0");fs::create_directory_symlink("/dev",sys/"class/nfc/nfc0");assert(radios(sys).empty());
        fs::remove_all(sys);std::cout<<"NFC: exact controller/rfkill association, per-index requests, hard block, ambiguity and containment refusal passed; no device write\n";
    }catch(...){fs::remove_all(sys);throw;}
}
