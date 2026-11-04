// SPDX-License-Identifier: MIT
#include "../src/rtc-counter-offset.hpp"
#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>

namespace cc=uke_counter_clock;
constexpr int64_t utc=1793826000;
template<class F> void reject(F action) {
    bool refused=false;try{action();}catch(const std::runtime_error &){refused=true;}assert(refused);
}
struct Model {
    uint64_t boot=5000000000, origin=boot, counter=1000;
    int64_t wall=utc;
    bool synchronized=true,stalled=false,reset=false,jump=false,set_fails=false,set_lies=false,bad_readback=false;
    unsigned clocks=0,reads=0,writes=0,sets=0,pauses=0;
    std::optional<cc::Record> saved;
    std::function<void(unsigned,cc::Sample &)> mutate=[](unsigned,cc::Sample &){};
    cc::Sample clock() {cc::Sample s{wall+int64_t((boot-origin)/1000000000),boot,synchronized};mutate(clocks++,s);return s;}
    uint64_t read() {
        ++reads;if(reset && reads>1)return counter-1;if(jump && reads>1)return counter+4;
        return counter+(stalled?0:(boot-origin)/1000000000);
    }
    void pause(){++pauses;boot+=250000000;}
    std::optional<cc::Record> load(){auto result=saved;if(result && writes && bad_readback)result->utc+=5;return result;}
    void save(cc::Record value){++writes;saved=value;}
    int set(int64_t value){++sets;if(set_fails)return -1;if(!set_lies)wall=value-int64_t((boot-origin)/1000000000);return 0;}
    cc::Result sync() {return cc::synchronize([&]{return read();},[&]{return clock();},[&]{return load();},
        [&](auto r){save(r);},[&]{pause();});}
    cc::Result restore(cc::Record r) {return cc::restore(r,[&]{return read();},[&]{return clock();},
        [&](int64_t v){return set(v);},[&]{pause();});}
};
int main() {
    static constexpr unsigned char vector[]="123456789";
    assert(cc::crc32(vector,9)==0xcbf43926);
    const cc::Record record{1000,utc};
    const auto wire=cc::encode(record);assert(cc::decode(wire)==record);
    for(size_t byte=0;byte<wire.size();++byte)for(unsigned bit=0;bit<8;++bit) {
        auto damaged=wire;damaged[byte]^=static_cast<unsigned char>(1U<<bit);reject([&]{cc::decode(damaged);});
    }
    for(const auto &r:{cc::Record{cc::counter_max+1,utc},cc::Record{0,cc::first_utc-1},cc::Record{0,cc::last_utc}})
        reject([&]{cc::encode(r);});
    assert(cc::mapped(record,1000)==utc && cc::mapped(record,1060)==utc+60);
    reject([&]{cc::mapped(record,999);});
    reject([&]{cc::mapped(record,cc::counter_max+1);});
    assert(cc::mapped({cc::counter_max-5,utc},cc::counter_max)==utc+5);
    reject([&]{cc::mapped({0,cc::last_utc-1},1);});
    {
        Model m;m.synchronized=false;assert(m.sync()==cc::Result::deferred && !m.reads && !m.writes);
        const cc::Record expected{1001,utc+1};
        m.synchronized=true;assert(m.sync()==cc::Result::saved && m.writes==1 && m.saved && *m.saved==expected);
        assert(m.sync()==cc::Result::unchanged && m.writes==1);
        m.saved=cc::Record{2000,utc};assert(m.sync()==cc::Result::saved && m.writes==2); // NTP repairs a reset anchor.
        m.saved=cc::Record{0,cc::last_utc-1};assert(m.sync()==cc::Result::saved && m.writes==3); // A stale overflowing mapping is reprovisioned.
    }
    for(unsigned fault=0;fault<3;++fault) {
        Model m;m.stalled=fault==0;m.reset=fault==1;m.jump=fault==2;
        reject([&]{m.sync();});assert(!m.writes && !m.sets);
    }
    for(unsigned stage=1;stage<=3;++stage) {
        Model m;m.mutate=[=](unsigned n,cc::Sample &s){if(n==stage)s.synchronized=false;};
        reject([&]{m.sync();});assert(!m.writes);
    }
    for(unsigned fault=0;fault<5;++fault) {
        Model m;m.mutate=[=](unsigned n,cc::Sample &s){if(n==2) {
            if(fault==0)--s.boot_ns;if(fault==1)s.boot_ns+=2000000000;
            if(fault==2)--s.utc;if(fault==3)s.utc+=3;if(fault==4)s.utc=INT64_MAX;
        }};
        reject([&]{m.sync();});assert(!m.writes);
    }
    {
        Model m;m.bad_readback=true;reject([&]{m.sync();});assert(m.writes==1);
        Model changed;changed.mutate=[](unsigned n,cc::Sample &s){if(n==4)s.synchronized=false;};
        reject([&]{changed.sync();});assert(changed.writes==1);
        Model failed;reject([&]{cc::synchronize([&]{return failed.read();},[&]{return failed.clock();},
            [&]{return failed.load();},[](cc::Record){throw std::runtime_error("injected durable write failure");},[&]{failed.pause();});});
        assert(!failed.saved && !failed.writes);
    }
    {
        Model m;m.synchronized=false;m.wall=utc-5000;
        assert(m.restore(record)==cc::Result::restored && m.sets==1 && !m.writes);
        assert(m.clock().utc==cc::mapped(record,m.read()));
        Model current;current.synchronized=false;current.wall=utc+60;
        assert(current.restore(record)==cc::Result::unchanged && !current.sets);
        Model online;assert(online.restore(record)==cc::Result::deferred && !online.reads && !online.sets);
        Model late;late.synchronized=false;late.wall=utc-5000;late.boot=late.origin=1800000000000;
        assert(late.restore(record)==cc::Result::restored && late.sets==1); // Slow offline boots still need elapsed UTC.
    }
    for(unsigned stage=1;stage<=2;++stage) {
        Model m;m.synchronized=false;m.wall=utc-5000;
        m.mutate=[=](unsigned n,cc::Sample &s){if(n==stage)s.synchronized=true;};
        assert(m.restore(record)==cc::Result::deferred && !m.sets);
    }
    for(unsigned fault=0;fault<6;++fault) {
        Model m;m.synchronized=false;m.wall=utc-5000;
        m.stalled=fault==0;m.reset=fault==1;m.jump=fault==2;m.set_fails=fault==3;m.set_lies=fault==4;
        if(fault==5)m.mutate=[](unsigned n,cc::Sample &s){if(n==2)s.utc-=1;};
        reject([&]{m.restore(record);});assert(m.sets==(fault==3 || fault==4?1U:0U) && !m.writes);
    }
    {
        Model m;m.synchronized=false;m.wall=0; // The raw counter calendar may begin in 1970.
        assert(m.restore(record)==cc::Result::restored && m.sets==1);
        Model elapsed;elapsed.synchronized=false;elapsed.wall=0;elapsed.counter=4600;
        assert(elapsed.restore(record)==cc::Result::restored && elapsed.clock().utc==utc+3601);
        Model reset;reset.synchronized=false;reset.wall=utc-5000;
        reject([&]{reset.restore({2000,utc});});assert(!reset.sets);
    }
    char pattern[]="/tmp/uke-counter-clock.XXXXXX";const char *temporary=mkdtemp(pattern);assert(temporary);
    const std::filesystem::path root=temporary;const auto owner=geteuid();
    cc::Fd dir(open(root.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));
    assert(!cc::read_record(dir.value,owner));
    cc::write_record(dir.value,record,owner);assert(cc::read_record(dir.value,owner)==record);
    struct stat st{};assert(fstatat(dir.value,cc::record_name,&st,AT_SYMLINK_NOFOLLOW)==0 && st.st_size==32 && (st.st_mode&0777)==0600);
    reject([&]{cc::read_record(dir.value,owner+1);});
    for(size_t byte=0;byte<wire.size();++byte) {
        auto damaged=wire;damaged[byte]^=1;
        {std::ofstream out(root/cc::record_name,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char *>(damaged.data()),damaged.size());assert(out);}
        reject([&]{cc::read_record(dir.value,owner);});assert(!cc::read_for_sync(dir.value,owner));
        cc::write_record(dir.value,record,owner);assert(cc::read_record(dir.value,owner)==record);
    }
    {
        std::ofstream out(root/cc::record_name,std::ios::trunc);out<<"short";
    }
    reject([&]{cc::read_record(dir.value,owner);});assert(!cc::read_for_sync(dir.value,owner));
    cc::write_record(dir.value,record,owner);
    assert(chmod((root/cc::record_name).c_str(),0644)==0);
    reject([&]{cc::read_record(dir.value,owner);});reject([&]{cc::write_record(dir.value,record,owner);});
    std::filesystem::remove(root/cc::record_name);
    {std::ofstream out(root/"outside");out<<"preserved";}
    assert(chmod((root/"outside").c_str(),0600)==0);
    std::filesystem::create_symlink(root/"outside",root/cc::record_name);
    reject([&]{cc::read_record(dir.value,owner);});reject([&]{cc::write_record(dir.value,record,owner);});
    std::filesystem::remove(root/cc::record_name);
    std::filesystem::create_hard_link(root/"outside",root/cc::record_name);
    reject([&]{cc::read_record(dir.value,owner);});reject([&]{cc::write_record(dir.value,record,owner);});
    std::filesystem::remove(root/cc::record_name);
    assert(mkfifo((root/cc::record_name).c_str(),0600)==0);
    reject([&]{cc::read_record(dir.value,owner);});reject([&]{cc::write_record(dir.value,record,owner);});
    std::filesystem::remove(root/cc::record_name);
    cc::write_record(dir.value,record,owner);
    assert(chmod(root.c_str(),0777)==0);reject([&]{cc::write_record(dir.value,record,owner);});
    assert(chmod(root.c_str(),0700)==0);
    std::string preserved;{std::ifstream in(root/"outside");in>>preserved;}assert(preserved=="preserved");
    for(const auto &entry:std::filesystem::directory_iterator(root))assert(entry.path().filename().string().find(".rtc-counter-offset.")!=0);
    std::filesystem::remove_all(root);
    std::cout<<"Counter UTC: standard CRC vector, 256 wire mutations, elapsed offline time including slow boots, reset/wrap/stop refusal, trusted-time races, forward restore and readback passed; real private POSIX state rejects unsafe paths and repairs data only through the synchronized writer.\n";
}
