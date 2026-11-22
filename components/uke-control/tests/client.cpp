// SPDX-License-Identifier: MIT
#include "client.hpp"
#include "tui-text.hpp"
#include <json/json.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <clocale>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
namespace client=uke::client;
using namespace std::chrono_literals;
template<class Fn> void refuses(Fn fn){bool caught=false;try{fn();}catch(const std::exception&){caught=true;}assert(caught);}
Json::Value json(const std::string& text){Json::Value value;Json::CharReaderBuilder builder;std::string error;const auto reader=std::unique_ptr<Json::CharReader>(builder.newCharReader());assert(reader->parse(text.data(),text.data()+text.size(),&value,&error));return value;}
std::string sample(client::Kind kind) {
    using K=client::Kind;
    switch(kind){case K::path:return "test path/ölçüm.json";case K::absolute_path:return "/tmp/review.json";case K::identifier:return "usb-device-1-1";case K::interface:return "enp0s1";case K::usb_device:return "1-2.3";case K::input_device:return "/dev/input/event1";case K::partition_uuid:return "01234567-89ab-cdef-0123-456789abcdef";case K::color:return "#01aB23";case K::swap_size:return "half";case K::repository:return "owner/project";default:assert(false);return {};}
}
void catalog() {
    std::set<std::string> groups,ids,commands;
    for(const auto& c:client::categories()){assert(groups.insert(c.id).second);assert(!c.title.en.empty()&&!c.title.tr.empty());assert(!c.description.en.empty()&&!c.description.tr.empty());}
    for(const auto& a:client::actions()) {
        assert(ids.insert(a.id).second&&groups.contains(a.category));assert(!a.title.en.empty()&&!a.title.tr.empty());assert(!a.description.en.empty()&&!a.description.tr.empty());
        assert(!a.arguments.empty());commands.insert(a.arguments[0]);
        if(a.effect!=client::Effect::read)assert(a.presentation==client::Presentation::terminal);
        if(a.presentation==client::Presentation::capture)assert(a.effect==client::Effect::read&&a.timeout_seconds>=1&&a.timeout_seconds<=630);
        auto values=client::defaults(a);std::set<std::string> parameters;
        for(const auto& p:a.parameters) {
            assert(parameters.insert(p.id).second);assert(!p.title.en.empty()&&!p.title.tr.empty()&&!p.description.en.empty()&&!p.description.tr.empty());
            if(p.kind==client::Kind::choice){std::set<std::string> choices;assert(!p.choices.empty());for(const auto& c:p.choices){assert(choices.insert(c.value).second);assert(!c.title.en.empty()&&!c.title.tr.empty()&&!c.description.en.empty()&&!c.description.tr.empty());}}
            if(values.at(p.id).empty()&&!p.optional)values[p.id]=sample(p.kind);
        }
        const auto p=client::plan(a.id,values);assert(p.confirmation_required==(a.effect!=client::Effect::read));assert(!p.argv.empty());
        for(const auto& argument:p.argv)assert(argument.find('{')==argument.npos);
        const auto data=json(client::plan_json(p));assert(!data["executed"].asBool()&&data["physical_acceptance"].isNull());assert(data["argv"].size()==p.argv.size());
        for(const auto* related:{&a.inspect,&a.preview})if(!related->empty())assert(client::action(*related).effect==client::Effect::read||(*related==a.preview&&client::action(*related).effect==client::Effect::local_file));
    }
    for(const auto* command:{"status","overview","battery","charge","usb","display","ethernet","cdc","desktop","flashlight","led","sensors","hardware","firmware","nfc","wake","inventory","roots","swap","resize","singleboot","multiboot","channel","update","compare","support","diagnostics","report","test","actions"})assert(commands.contains(command));
    const auto data=json(client::catalog_json());assert(data["api_version"]==1&&data["metadata_only"]==true);assert(data["actions"].size()==ids.size());
    const auto search=client::search("ŞARJ");assert(std::find(search.begin(),search.end(),"charge.mode")!=search.end());assert(client::search("sarj").size()==search.size());assert(client::search("İŞ YÜKÜ").size()>0);
    std::cout<<"catalog: "<<ids.size()<<" actions / "<<groups.size()<<" categories\n";
}
void validation() {
    refuses([]{client::plan("no-such-action",{});});
    refuses([]{client::plan("status",{{"program","/bin/sh"}});});
    refuses([]{client::plan("charge.limit",{{"percent","101"}});});
    refuses([]{client::plan("charge.limit",{{"percent","18446744073709551616"}});});
    refuses([]{client::plan("charge.mode",{{"mode","firmware; reboot"}});});
    const auto& path=client::action("snapshot.save-all").parameters[0];
    for(const auto& value:std::vector<std::string>{"--yes",std::string("x\0y",3),"x\ny","x\x1b[2J","x\xc2\x9by","x\xe2\x80\xaey",std::string(4097,'a'),std::string(1,char(0xff))})refuses([&]{client::validate_value(path,value);});
    const std::string literal="./ölçüm {literal} $(touch NEVER) 'quote'.json";
    const auto p=client::plan("snapshot.save-all",{{"file",literal}});assert(p.argv.back()==literal);
    const auto& iface=client::action("ethernet.status").parameters[0];for(const auto* value:{"../../eth0","a b","0123456789012345"})refuses([&]{client::validate_value(iface,value);});
    const auto& color=client::action("led.color").parameters[0];refuses([&]{client::validate_value(color,"red");});
    auto forged=p;forged.argv={"/bin/sh","-c","false"};assert(json(client::plan_json(forged))["argv"][0].asString()!="/bin/sh");
    refuses([]{client::Query q("charge.mode");});
    refuses([]{client::run_terminal("charge.mode",{},false);});
}
client::Result run(const char* mode,bool cancel=false) {
    setenv("UKE_CLIENT_STUB_MODE",mode,1);client::Query query("status");const auto began=std::chrono::steady_clock::now();
    while(query.result().running) {
        query.poll();if(cancel&&query.result().output.find("ready")!=std::string::npos)query.cancel();
        assert(std::chrono::steady_clock::now()-began<25s);std::this_thread::sleep_for(5ms);
    }
    return query.result();
}
void processes() {
    const auto echo=run("echo");assert(echo.exit_status==7&&!echo.cancelled&&!echo.truncated);assert(echo.error=="separate error stream\n");assert(echo.output.find("separate error")==echo.output.npos);assert(json(echo.output)["cycles"]==0);
    const auto cancelled=run("hang",true);assert(cancelled.cancelled&&cancelled.exit_status==130&&cancelled.elapsed_ms<2000);
    const auto descendant=run("descendant",true);assert(descendant.cancelled&&descendant.exit_status==130&&descendant.elapsed_ms<2000);
    const auto overflow=run("overflow");assert(overflow.truncated&&overflow.exit_status==124&&overflow.output.size()==1024*1024);
    const auto timed=run("hang");assert(timed.timed_out&&timed.exit_status==124&&!timed.cancelled&&timed.elapsed_ms>=20000&&timed.elapsed_ms<24000);
    setenv("UKE_CLIENT_STUB_MODE","hang",1);const auto began=std::chrono::steady_clock::now();{client::Query query("status");}assert(std::chrono::steady_clock::now()-began<2s);
}
void output_and_files() {
    const auto readable=client::format_output("{\"zero\":0,\"negative\":-42,\"unknown\":null,\"available\":false}",client::Language::english);
    assert(readable.find("zero: 0")!=readable.npos&&readable.find("negative: -42")!=readable.npos&&readable.find("Not reported")!=readable.npos&&readable.find("No")!=readable.npos);
    assert(client::format_output("bad\xff",client::Language::turkish).find(char(0xff))==std::string::npos);
    namespace t=uke::tui_text;
    const auto malicious=t::wide("x\x1b[2J\xe2\x80\xaey");assert(malicious.find(L'\x1b')==malicious.npos&&malicious.find(L"\\u001b")!=malicious.npos&&malicious.find(L"\\u202e")!=malicious.npos);
    assert(t::cells(t::wide("Ş电"))==3&&t::utf8(t::clipped(t::wide("Ş电x"),3))=="Ş电");
    for(const auto& line:t::wrap("şarj 电池 long long\n\nend",5))assert(t::cells(line)<=5);
    assert(t::wrap(std::string(200,'x'),2,3).size()==4);
    char directory[]="/tmp/uke-client-XXXXXX";assert(mkdtemp(directory));const auto path=std::string(directory)+"/receipt.json";
    client::Result result;result.action_id="status";result.output="{\"cycles\":0}";result.exit_status=0;
    client::save_receipt(path,result,{});struct stat st{};assert(!stat(path.c_str(),&st)&&(st.st_mode&0777)==0600);
    refuses([&]{client::save_receipt(path,result,{});});assert(!symlink(path.c_str(),(path+".link").c_str()));refuses([&]{client::save_receipt(path+".link",result,{});});
    result.running=true;refuses([&]{client::save_receipt(path+".unfinished",result,{});});
    unlink((path+".link").c_str());unlink(path.c_str());rmdir(directory);
}
int main(){assert(setlocale(LC_ALL,"C.UTF-8"));catalog();validation();processes();output_and_files();std::cout<<"client validation, process ownership, deadlines and receipts passed\n";}
