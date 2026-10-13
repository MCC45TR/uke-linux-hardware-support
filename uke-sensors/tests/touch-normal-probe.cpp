// SPDX-License-Identifier: GPL-2.0-or-later
#include <cstdarg>
#include <sys/ioctl.h>
int mock_ioctl(int,unsigned long,...) noexcept;
#define ioctl mock_ioctl
#define UKE_TOUCH_NORMAL_TEST
#include "../src/touch-normal-probe.cpp"
#undef ioctl

unsigned operations,fail_at=~0U,selected,status_reads;
bool never_copy,never_reset,bad_crc;
bool bad_bank;
bool bad_metadata;
std::array<std::uint8_t,0x120000> ram{};
std::vector<std::pair<unsigned,Bytes>> writes;
int mock_ioctl(int,unsigned long request,...) noexcept {
    if(operations++==fail_at) return -1;
    if(request!=SPI_IOC_MESSAGE(1)) return -1;
    va_list args; va_start(args,request); auto *m=va_arg(args,spi_ioc_transfer *); va_end(args);
    auto *tx=reinterpret_cast<const std::uint8_t *>(m->tx_buf);
    auto *rx=reinterpret_cast<std::uint8_t *>(m->rx_buf);
    if(m->speed_hz!=1000000 || m->bits_per_word!=8) return -1;
    if(!rx && m->len==3 && tx[0]==0xff) selected=(unsigned(tx[1])<<15)|(unsigned(tx[2])<<7);
    else if(!rx) {
        auto address=selected+(tx[0]&0x7f);
        writes.emplace_back(address,Bytes(tx+1,tx+m->len));
        if(address<ram.size() && m->len-1<=ram.size()-address)
            std::copy(tx+1,tx+m->len,ram.begin()+address);
    }
    else {
        std::fill(rx,rx+m->len,0);
        const auto address=selected+(tx[0]&0x7f);
        if(address<0x1142a0 && address!=0x11c478 && m->len-2<=ram.size()-address) {
            std::copy(ram.begin()+address,ram.begin()+address+m->len-2,rx+2); if(bad_bank) rx[2]^=1;
        }
        else if(address==0x1fc925) { rx[2]=never_copy ? 1 : 0; status_reads++; }
        else if(address==0x11c460) { rx[2]=never_reset ? 0 : 0xa0; status_reads++; }
        else if(address==0x11c450) rx[2]=0;
        else if(address==0x1093d8 && m->len==41) { rx[4]=0x38; rx[5]=0x14; }
        else if(address==0x11c400 && m->len==5434) rx[2]=0xff;
        else if(address==0x1fb520) {
            const Bytes crc{0xf0,0xd7,0xbd,0xe4}; std::copy(crc.begin(),crc.end(),rx+2); if(bad_crc) rx[2]^=1;
        } else if(address==0x1fb524) {
            const Bytes crc{0x47,0x52,0x49,0x89}; std::copy(crc.begin(),crc.end(),rx+2);
        } else if(address==0x11c478) {
            const Bytes info{0x11,0xee,60,40,0x7d,0,0x53,0x70}; std::copy(info.begin(),info.end(),rx+2);
            if(bad_metadata) rx[4]^=1;
        } else return -1;
    }
    return m->len;
}
void reset() { operations=status_reads=selected=0; fail_at=~0U; never_copy=never_reset=bad_crc=bad_bank=bad_metadata=false; writes.clear(); }
template<class F> void reject(F f) { bool refused=false; try{f();}catch(const std::exception&){refused=true;} assert(refused); }
int main(int argc,char **argv) {
    assert(argc==2);
    std::ifstream input(argv[1],std::ios::binary);
    Bytes data{std::istreambuf_iterator<char>(input),{}};
    normal_image(data);
    auto bad=data; bad[1000]^=1; reject([&]{normal_image(bad);});
    bad=data; bad[8]^=1; reject([&]{normal_image(bad);});
    bad.resize(100); reject([&]{normal_image(bad);});
    reset(); load_normal(0,data); const unsigned complete=operations;
    assert(writes.front()==std::make_pair(0x1fb43eU,Bytes{0x69}));
    assert(writes.back()==std::make_pair(0x1fb50dU,Bytes{1}) && status_reads==2);
    std::size_t bank=0,offset=0;
    for(std::size_t i=8;i<writes.size()-3;i++) {
        const auto &b=banks[bank]; const auto &[address,bytes]=writes[i];
        assert(address==b.address+offset && bytes.size()<=1024);
        assert(std::equal(bytes.begin(),bytes.end(),data.begin()+b.offset+offset));
        offset+=bytes.size();
        if(offset==b.size) { bank++; offset=0; }
    }
    assert(bank==4 && offset==0);
    for(unsigned fault=0;fault<complete;fault++) {
        reset(); fail_at=fault; reject([&]{load_normal(0,data);}); assert(operations==fault+1);
    }
    reset(); reject([&]{load_normal(0,bad);}); assert(operations==0);
    reset(); reject([&]{normal_write(0,0x1fb43e,Bytes{0,0});}); assert(operations==0);
    reject([&]{normal_write(0,0x3f0fe,Bytes{0x69});}); assert(operations==0);
    reject([&]{normal_write(0,0x200000,Bytes{1});}); assert(operations==0);
    reject([&]{normal_write(0,0x11c450,Bytes{0xbf,0x0c,0,1,0});}); assert(operations==0);
    reject([&]{normal_read<1>(0,0x1fb43e);}); assert(operations==0);
    reset(); bad_crc=true; load_normal(0,data); // Post-boot result slots do not establish programming integrity.
    reset(); bad_bank=true; reject([&]{load_normal(0,data);});
    for(const auto &[address,bytes] : writes) { (void)bytes; assert(address!=0x1fb50d); }
    reset(); bad_metadata=true; reject([&]{load_normal(0,data);});
    reset(); never_copy=true; reject([&]{load_normal(0,data);}); assert(status_reads==200);
    reset(); never_reset=true; reject([&]{load_normal(0,data);}); assert(status_reads==51);
    reset(); inspect_normal(0); assert(writes.empty());
    reset(); probe_coordinates(0);
    assert(writes.size()==2 && writes[0].second==Bytes({0xbf,0x15,0,1,0}) &&
           writes[1].second==Bytes({0xbf,0x15,0,0,0}));
    reset(); fail_at=10; reject([&]{probe_coordinates(0);});
    assert(writes.back().second==Bytes({0xbf,0x15,0,0,0}));
    (void)&compatible; (void)&nt36532;
    std::cout<<"Pinned normal image, exact bank bytes, all transfer faults, CRC and bounded timeout guards passed\n";
}
