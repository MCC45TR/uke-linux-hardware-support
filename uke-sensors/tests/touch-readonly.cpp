// SPDX-License-Identifier: MIT
#define UKE_TOUCH_TEST
#include "../src/touch-readonly.cpp"
#include <cstdlib>
int main() {
    assert(matches(std::string("other\0xiaomi,uke\0",17),"xiaomi,uke"));
    assert(!matches(std::string("not-xiaomi,uke\0",15),"xiaomi,uke"));
    assert(!matches("xiaomi,uke","xiaomi,uke"));
    assert(!matches(std::string("xiaomi,uke\0unterminated",23),"xiaomi,uke"));
    char path[]="/tmp/uke-touch-identity.XXXXXX";
    const int first=mkstemp(path); assert(first>=0);
    {
        std::string value(4084,'a'); value.push_back('\0'); value.append("xiaomi,uke"); value.push_back('\0');
        assert(value.size()==4096);
        std::ofstream file(path,std::ios::binary); file.write(value.data(),value.size()); file.close();
        assert(compatible(path,"xiaomi,uke"));
        std::ofstream extra(path,std::ios::binary|std::ios::app); extra.put('\0'); extra.close();
        assert(!compatible(path,"xiaomi,uke"));
    }
    lock_touch(first);
    const int second=open(path,O_RDWR|O_CLOEXEC); assert(second>=0);
    bool busy=false; try{lock_touch(second);}catch(const std::exception&){busy=true;}
    assert(busy);
    assert(close(first)==0);
    lock_touch(second);
    assert(close(second)==0 && unlink(path)==0);
    assert((page(0x1fb104)==std::array<std::uint8_t,3>{0xff,0x3f,0x62}));
    assert((page(0x125878)==std::array<std::uint8_t,3>{0xff,0x24,0xb0}));
    assert((page(0x1fb12c)==std::array<std::uint8_t,3>{0xff,0x3f,0x62}));
    assert((page(0x11c478)==std::array<std::uint8_t,3>{0xff,0x23,0x88}));
    assert(firmware_address(0)==0x11c478);
    assert(firmware_address(0xfe)==0x11c478);
    assert(firmware_address(1)==0x125878);
    assert(firmware_address(0xff)==0x125878);
    bool rejected=false; try{page(0x1fb43e);}catch(const std::exception&){rejected=true;}
    assert(rejected); // Software-reset address is explicitly outside this tool.
    assert(nt36532({0xff,0xff,0xff,0x32,0x65,3}));
    assert(!nt36532({0xff,0xff,0xff,0x23,0x65,3}));
    std::array<std::uint8_t,16> info{1,254,10,10,8,88,12,128};
    firmware(info); info[1]=0;
    rejected=false; try{firmware(info);}catch(const std::exception&){rejected=true;}
    assert(rejected);
    info={0x11,0xee,60,40,0x7d,0,0x53,0x70}; firmware(info);
    info[6]=info[7]=0;
    rejected=false; try{firmware(info);}catch(const std::exception&){rejected=true;}
    assert(rejected);
    (void)&read<6>;
    std::cout << "Fixed touch identity, bounded full-list validation, exclusive cooperative transport, reset rejection and firmware-info guards passed\n";
}
