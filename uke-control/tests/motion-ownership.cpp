// SPDX-License-Identifier: MIT
#include "motion-ownership.hpp"
#include <cassert>
#include <iostream>
#include <map>
#ifdef NDEBUG
#error Assertions are required
#endif
using namespace uke::motion;
namespace {
struct Fake:Store {
    std::map<std::string,std::string> values;
    std::vector<Setting> journal;
    std::vector<std::string> writes;
    int fail_write=-1;
    bool fail_save=false,fail_clear=false;
    std::optional<std::string> read(std::string_view key)override {
        const auto it=values.find(std::string(key));return it==values.end()?std::optional<std::string>{}:it->second;
    }
    bool write(std::string_view key,std::string_view value)override {
        assert(!journal.empty());writes.emplace_back(key);
        if(int(writes.size())==fail_write)return false;
        values[std::string(key)]=value;return true;
    }
    bool save(const std::vector<Setting>& settings)override {if(fail_save)return false;journal=settings;return true;}
    bool clear()override {if(fail_clear)return false;journal.clear();return true;}
};
const std::vector<Setting> settings{{"current_timestamp_clock","monotonic","boottime"},{"buffer/length","2","4"},
    {"scan_elements/in_timestamp_en","0","1"},{"buffer/enable","0","1"}};
Fake fresh() {Fake f;for(const auto& s:settings)f.values[s.path]=s.before;return f;}
}
int main() {
    assert(settings_valid(settings));
    for(auto path:{"/buffer/enable","../buffer/enable","scan_elements/../enable_en","scan_elements/in_temp_en","sampling_frequency"})assert(!setting_path(path));
    auto bad=settings;bad[0].path="../enable";assert(!settings_valid(bad));
    bad=settings;bad[0].before="monotonic\nboottime";assert(!settings_valid(bad));
    bad=settings;bad.push_back(settings.back());assert(!settings_valid(bad));
    {
        auto f=fresh();assert(configure(f,settings));assert(f.writes.back()=="buffer/enable");
        f.writes.clear();assert(restore(f,settings));assert(f.writes.front()=="buffer/enable");assert(f.journal.empty());
        for(const auto& s:settings)assert(f.values[s.path]==s.before);
    }
    for(int failure=1;failure<=int(settings.size());++failure){
        auto f=fresh();f.fail_write=failure;assert(!configure(f,settings));
        for(const auto& s:settings)assert(f.values[s.path]==s.before);assert(f.journal.empty());
    }
    for(std::size_t crash=0;crash<=settings.size();++crash){
        auto f=fresh();assert(f.save(settings));
        for(std::size_t i=0;i<crash;++i)f.values[settings[i].path]=settings[i].after;
        assert(restore(f,settings));for(const auto& s:settings)assert(f.values[s.path]==s.before);
    }
    {
        auto f=fresh();f.fail_save=true;assert(!configure(f,settings));assert(f.writes.empty());
        f=fresh();f.values["buffer/enable"]="1";assert(!configure(f,settings));assert(f.writes.empty()&&f.journal.empty());
        f=fresh();assert(configure(f,settings));f.values["buffer/length"]="99";f.writes.clear();
        assert(!restore(f,settings));assert(f.writes.empty()&&!f.journal.empty());
    }
    {
        auto f=fresh();assert(configure(f,settings));f.fail_write=int(f.writes.size())+1;
        assert(!restore(f,settings));assert(!f.journal.empty()&&f.values["buffer/enable"]=="1");
        f.fail_write=-1;f.fail_clear=true;assert(!restore(f,settings));assert(!f.journal.empty());
        f.fail_clear=false;assert(restore(f,settings));assert(f.journal.empty());
    }
    std::cout<<"Motion enable-last ownership, partial setup, crash recovery and foreign-state refusal passed\n";
}
