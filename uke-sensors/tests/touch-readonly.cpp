// SPDX-License-Identifier: MIT
#define UKE_TOUCH_TEST
#include "../src/touch-readonly.cpp"
int main() {
    assert(matches(std::string("other\0xiaomi,uke\0",17),"xiaomi,uke"));
    assert(!matches(std::string("not-xiaomi,uke\0",15),"xiaomi,uke"));
    assert(!matches("xiaomi,uke","xiaomi,uke"));
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
    (void)&compatible; (void)&read<6>;
    std::cout << "Fixed touch identity reads, reset rejection and firmware-info guards passed\n";
}
