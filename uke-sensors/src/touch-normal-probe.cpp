// SPDX-License-Identifier: GPL-2.0-or-later
// Normal RAM-download protocol: pinned MiCode Pipa/Piano sources, checked against the Uke OEM module.
#define UKE_TOUCH_TEST
#include "touch-readonly.cpp"
#include <chrono>
#include <openssl/sha.h>
#include <span>
#include <thread>
#include <vector>

using Bytes = std::vector<std::uint8_t>;
struct Bank { unsigned offset, address, size; };
static constexpr std::array<Bank,4> banks{{
    {0x200,0,0x174d8}, {0x22000,0x100000,0x140a0},
    {0,0x1140a0,0x100}, {0x100,0x1141a0,0x100}
}};
static unsigned word(std::span<const std::uint8_t> data, unsigned offset) {
    require(offset <= data.size() && data.size()-offset >= 4, "Truncated normal firmware header");
    return unsigned(data[offset]) | (unsigned(data[offset+1])<<8) |
           (unsigned(data[offset+2])<<16) | (unsigned(data[offset+3])<<24);
}
static void normal_image(std::span<const std::uint8_t> data) {
    // shortcut: this probe admits only the measured O82 CSOT image; extend in the kernel port after qualification.
    require(data.size()==245760, "Unexpected normal firmware length");
    require(word(data,0)==0x200 && word(data,4)==0 && word(data,8)==0x174d7 &&
            word(data,12)==0x22000 && word(data,16)==0x100000 && word(data,20)==0x1409f &&
            word(data,24)==0xe4bdd7f0 && word(data,28)==0x89495247 &&
            data[0x20]==3 && data[0x28]==0 && data[0x29]==0 &&
            word(data,0xf0)==0x1140a0 && word(data,0xf4)==0xff && word(data,0xf8)==0 &&
            word(data,0x1f0)==0x1141a0 && word(data,0x1f4)==0xff && word(data,0x1f8)==0x100,
            "Unsupported normal firmware layout");
    for(const auto &b : banks)
        require(b.offset <= data.size() && b.size <= data.size()-b.offset, "Normal firmware bank exceeds file");
    std::array<unsigned char,SHA256_DIGEST_LENGTH> digest{};
    require(SHA256(data.data(),data.size(),digest.data())!=nullptr, "Firmware hash unavailable");
    static constexpr std::array<unsigned char,32> expected{
        0xa4,0xf9,0x49,0xfb,0x89,0xc7,0xea,0xad,0x9d,0xa8,0x5a,0x58,0x85,0xcd,0x2d,0xcc,
        0x1f,0xff,0xbf,0x08,0xbc,0x1e,0x7f,0xfe,0x9f,0x0b,0xb9,0x7c,0x17,0x69,0xb3,0x10};
    require(digest==expected, "Normal CSOT firmware SHA-256 mismatch");
}
static std::array<std::uint8_t,3> select_page(unsigned address) {
    return {0xff,std::uint8_t(address>>15),std::uint8_t(address>>7)};
}
static void normal_write(int fd, unsigned address, std::span<const std::uint8_t> data) {
    bool allowed = false;
    for(const auto &b : banks)
        allowed |= address>=b.address && address-b.address<=b.size && data.size()<=b.size-(address-b.address);
    for(const auto &[reg,size] : std::array<std::pair<unsigned,unsigned>,10>{{
        {0x1fb43e,1},{0x1fb528,3},{0x1fb518,3},{0x1fb500,4},{0x1fb52c,3},
        {0x1fb530,3},{0x1fb504,4},{0x1fc925,1},{0x11c460,6},{0x11c450,2}}})
        allowed |= address==reg && data.size()==size;
    allowed |= address==0x1fb50d && data.size()==1;
    allowed |= address==0x11c450 && data.size()==5 && data[0]==0xbf && data[1]==0x15 &&
               data[2]==0 && data[3]<=1 && data[4]==0;
    require(allowed && !data.empty() && data.size()<=1024, "Write outside normal RAM-download scope");
    auto select=select_page(address); transfer(fd,select.data(),nullptr,select.size());
    Bytes tx(data.size()+1); tx[0]=(address&0x7f)|0x80;
    std::copy(data.begin(),data.end(),tx.begin()+1); transfer(fd,tx.data(),nullptr,tx.size());
}
template<std::size_t N> static std::array<std::uint8_t,N> normal_read(int fd,unsigned address) {
    require((N==1 && (address==0x1fc925 || address==0x11c460 || address==0x11c450)) ||
            (N==4 && (address==0x1fb520 || address==0x1fb524)) ||
            (N==39 && address==0x1093d8), "Read outside normal status scope");
    auto select=select_page(address); transfer(fd,select.data(),nullptr,select.size());
    std::array<std::uint8_t,N+2> tx{},rx{}; tx[0]=address&0x7f;
    transfer(fd,tx.data(),rx.data(),tx.size());
    std::array<std::uint8_t,N> result{}; std::copy(rx.begin()+2,rx.end(),result.begin()); return result;
}
static void pause_ms(unsigned milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}
static void verify_bank(int fd,unsigned address,std::span<const std::uint8_t> expected) {
    bool allowed=false;
    for(const auto &b : banks)
        allowed |= address>=b.address && address-b.address<=b.size && expected.size()<=b.size-(address-b.address);
    require(allowed && !expected.empty() && expected.size()<=1024,"Readback outside normal bank scope");
    auto select=select_page(address); transfer(fd,select.data(),nullptr,select.size());
    Bytes tx(expected.size()+2),rx(tx.size()); tx[0]=address&0x7f;
    transfer(fd,tx.data(),rx.data(),tx.size());
    require(std::equal(expected.begin(),expected.end(),rx.begin()+2),"Normal firmware bank readback mismatch");
}
static void inspect_normal(int fd) {
    std::cout<<"Reset state="<<std::hex<<unsigned(normal_read<1>(fd,0x11c460)[0])
             <<" Auto-copy="<<unsigned(normal_read<1>(fd,0x1fc925)[0])
             <<" ILM="<<word(normal_read<4>(fd,0x1fb520),0)
             <<" DLM="<<word(normal_read<4>(fd,0x1fb524),0)<<std::dec<<'\n';
    const auto info=read<16>(fd,firmware_address(0));
    std::cout<<"Firmware-info prefix:"<<std::hex;
    for(unsigned i=0;i<8;i++) std::cout<<' '<<unsigned(info[i]);
    std::cout<<std::dec<<'\n';
    firmware(info);
}
static void coordinate_mode(int fd,bool enabled) {
    normal_write(fd,0x11c450,Bytes{0xbf,0x15,0,std::uint8_t(enabled),0});
    for(unsigned attempt=0;attempt<10;attempt++) {
        pause_ms(10);
        if(normal_read<1>(fd,0x11c450)[0]==0) return;
    }
    throw std::runtime_error("Reviewed coordinate-mode command timed out");
}
static void probe_coordinates(int fd) {
    const auto info=read<16>(fd,firmware_address(0));
    require(info[0]==0x11 && info[1]==0xee && info[2]==60 && info[3]==40 &&
            info[4]==0x7d && info[5]==0 && info[6]==0x53 && info[7]==0x70,
            "Resident normal firmware not admitted for coordinate probe");
    const auto poll_info=normal_read<39>(fd,0x1093d8);
    const unsigned frame_length=unsigned(poll_info[2])|(unsigned(poll_info[3])<<8);
    require(frame_length==5176,"Unqualified THP frame length");
    std::cout<<"THP frame length="<<frame_length<<'\n';
    try {
        coordinate_mode(fd,true);
        for(unsigned sample=0;sample<8;sample++) {
            auto select=select_page(0x11c400); transfer(fd,select.data(),nullptr,select.size());
            Bytes tx(256+frame_length+2),rx(tx.size());
            transfer(fd,tx.data(),rx.data(),tx.size());
            std::array<unsigned char,SHA256_DIGEST_LENGTH> digest{};
            require(SHA256(rx.data()+2,rx.size()-2,digest.data())!=nullptr,"Frame digest unavailable");
            unsigned checksum=0; for(unsigned i=2;i<67;i++) checksum+=rx[i];
            std::cout<<"frame "<<sample<<" standard-checksum="<<((checksum&255)==0)<<" prefix:"<<std::hex;
            for(unsigned i=2;i<98;i++) std::cout<<' '<<unsigned(rx[i]);
            std::cout<<" full-sha256=";
            for(auto byte : digest) { static constexpr char hex[]="0123456789abcdef"; std::cout<<hex[byte>>4]<<hex[byte&15]; }
            std::cout<<std::dec<<'\n'; pause_ms(100);
        }
    } catch(...) {
        coordinate_mode(fd,false); throw;
    }
    coordinate_mode(fd,false);
}
static void load_normal(int fd, std::span<const std::uint8_t> data) {
    normal_image(data);
    normal_write(fd,0x1fb43e,Bytes{0x69}); pause_ms(5);
    normal_write(fd,0x1fb528,Bytes{0,0,0}); normal_write(fd,0x1fb518,Bytes{0xd7,0x74,1});
    normal_write(fd,0x1fb500,Bytes{0xf0,0xd7,0xbd,0xe4});
    normal_write(fd,0x1fb52c,Bytes{0,0,0x10}); normal_write(fd,0x1fb530,Bytes{0x9f,0x40,1});
    normal_write(fd,0x1fb504,Bytes{0x47,0x52,0x49,0x89});
    normal_write(fd,0x1fc925,Bytes{0x56});
    for(const auto &b : banks)
        for(unsigned offset=0;offset<b.size;) {
            auto count=std::min(1024U,b.size-offset);
            normal_write(fd,b.address+offset,data.subspan(b.offset+offset,count)); offset+=count;
        }
    unsigned attempt=0;
    for(;attempt<200;attempt++) {
        if(normal_read<1>(fd,0x1fc925)[0]==0) break;
        pause_ms(1);
    }
    require(attempt<200,"Normal cascade auto-copy timed out");
    for(const auto &b : banks)
        for(unsigned offset=0;offset<b.size;) {
            auto count=std::min(1024U,b.size-offset);
            verify_bank(fd,b.address+offset,data.subspan(b.offset+offset,count)); offset+=count;
        }
    std::cout<<"All four normal RAM banks match the pinned image before boot-ready\n";
    normal_write(fd,0x11c460,Bytes(6,0)); normal_write(fd,0x11c450,Bytes{0xae,0});
    normal_write(fd,0x1fb50d,Bytes{1}); pause_ms(5);
    for(attempt=0;attempt<50;attempt++) {
        auto status=normal_read<1>(fd,0x11c460)[0];
        if(status>=0xa0 && status<=0xaf) break;
        pause_ms(10);
    }
    require(attempt<50,"Normal firmware reset state timed out");
    const auto ilm_crc=word(normal_read<4>(fd,0x1fb520),0);
    const auto dlm_crc=word(normal_read<4>(fd,0x1fb524),0);
    std::cout<<"Hardware CRC: ILM="<<std::hex<<ilm_crc<<" DLM="<<dlm_crc<<std::dec<<'\n';
    /* Verify bytes before boot; the OEM accepts reset state, not post-boot CRC slots. */
    const auto info=read<16>(fd,firmware_address(0));
    firmware(info);
    require(info[0]==0x11 && info[1]==0xee && info[2]==60 && info[3]==40 &&
            info[4]==0x7d && info[5]==0 && info[6]==0x53 && info[7]==0x70,
            "Unexpected resident normal CSOT firmware metadata");
}
#ifndef UKE_TOUCH_NORMAL_TEST
int main(int argc,char **argv) {
    int fd=-1;
    try {
        require(argc==2 && (std::string(argv[1])=="--validate" || std::string(argv[1])=="--load-normal" || std::string(argv[1])=="--inspect" || std::string(argv[1])=="--probe-coordinates"),
                "Usage: uke-touch-normal-probe --validate|--load-normal|--inspect|--probe-coordinates; fixed /tmp/uke-touch-normal.bin");
        Bytes data(245760); std::ifstream input("/tmp/uke-touch-normal.bin",std::ios::binary);
        require(bool(input.read(reinterpret_cast<char *>(data.data()),data.size())) && input.peek()==EOF,
                "Normal firmware unavailable or unexpected length");
        normal_image(data);
        if(std::string(argv[1])=="--validate") { std::cout<<"Exact normal CSOT image and layout validated\n"; return 0; }
        require(compatible("/sys/firmware/devicetree/base/compatible","xiaomi,uke") &&
                compatible("/sys/bus/spi/devices/spi0.0/of_node/compatible","novatek,NVT-ts-spi"),"Foreign touch device");
        std::ifstream admission("/run/uke-touch-csot-admitted"); std::string token;
        require(bool(std::getline(admission,token)) && token=="uke-gpio26-input-high-cascade", "Supplier preflight missing");
        fd=open("/dev/spidev0.0",O_RDWR|O_CLOEXEC); require(fd>=0,"Reviewed SPI leaf unavailable");
        std::uint8_t mode=255;
        require(ioctl(fd,SPI_IOC_RD_MODE,&mode)==0 && mode==SPI_MODE_0,"Unexpected SPI mode");
        require(nt36532(read<6>(fd,0x1fb104)) && !(read<1>(fd,0x1fb12c)[0]&1),"Foreign chip/topology");
        if(std::string(argv[1])=="--inspect") { inspect_normal(fd); close(fd); return 0; }
        if(std::string(argv[1])=="--probe-coordinates") { probe_coordinates(fd); close(fd); return 0; }
        load_normal(fd,data); close(fd); fd=-1;
        std::cout<<"Normal RAM bank readback, reset state and resident firmware metadata passed; input remains unqualified\n";
        return 0;
    } catch(const std::exception &e) {
        if(fd>=0) close(fd);
        std::cerr<<e.what()<<'\n'; return 1;
    }
}
#endif
