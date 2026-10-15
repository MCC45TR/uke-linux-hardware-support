// SPDX-License-Identifier: MIT
// Fixed NT36532 identity reads; no reset, firmware download, GPIO or rail writes.
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <linux/spi/spidev.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <unistd.h>

static void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
static void lock_touch(int fd) {
    // Advisory ownership coordinates these tools; unrelated SPI clients must still be stopped.
    require(flock(fd,LOCK_EX|LOCK_NB)==0,"Touch SPI transport is already owned or cannot be locked");
}
static bool matches(const std::string &data, const std::string &expected) {
    if(data.size()>4096) return false;
    bool found=false;
    for(std::size_t p=0;p<data.size();) {
        auto end=data.find('\0',p);
        if(end==std::string::npos) return false;
        if(data.compare(p,end-p,expected)==0) found=true;
        p=end+1;
    }
    return found;
}
static bool compatible(const char *path, const std::string &expected) {
    std::ifstream input(path, std::ios::binary);
    if(!input) return false;
    std::array<char,4097> data{};
    input.read(data.data(),data.size());
    return input.eof() && !input.bad() && matches(std::string(data.data(),input.gcount()),expected);
}
static std::array<std::uint8_t,3> page(std::uint32_t address) {
    require(address == 0x1fb104 || address == 0x1fb12c || address == 0x125878 || address == 0x11c478, "Address outside fixed identity-read scope");
    return {0xff, std::uint8_t(address >> 15), std::uint8_t(address >> 7)};
}
static bool nt36532(const std::array<std::uint8_t,6> &id) {
    return id[3] == 0x32 && id[4] == 0x65 && id[5] == 3;
}
static std::uint32_t firmware_address(std::uint8_t topology) {
    return (topology & 1) ? 0x125878 : 0x11c478;
}
static void firmware(const std::array<std::uint8_t,16> &info) {
    require(unsigned(info[0]) + info[1] == 255, "Firmware-info complement check failed");
    auto x = (unsigned(info[4]) << 8) | info[5];
    auto y = (unsigned(info[6]) << 8) | info[7];
    require(x && y, "Firmware raw dimensions unavailable");
    std::cout << "NT36532 family, firmware " << unsigned(info[0]) << ", raw coordinates " << x << 'x' << y << '\n';
}
static void transfer(int fd, const void *tx, void *rx, unsigned size,unsigned speed=1000000) {
    require(speed==1000000 || (size==5434 && speed==19200000),"Unreviewed SPI transfer speed");
    spi_ioc_transfer message{};
    message.tx_buf = reinterpret_cast<std::uintptr_t>(tx);
    message.rx_buf = reinterpret_cast<std::uintptr_t>(rx);
    message.len = size; message.speed_hz = speed; message.bits_per_word = 8;
    const int result=ioctl(fd, SPI_IOC_MESSAGE(1), &message);
    if(result != int(size))
        throw std::runtime_error("SPI transfer size="+std::to_string(size)+
            " result="+std::to_string(result)+" errno="+
            (result<0 ? std::string(std::strerror(errno)) : std::string("short transfer")));
}
template<std::size_t N> static std::array<std::uint8_t,N> read(int fd, std::uint32_t address) {
    auto select = page(address); transfer(fd, select.data(), nullptr, select.size());
    std::array<std::uint8_t,N+2> tx{}, rx{};
    tx[0] = address & 0x7f;
    transfer(fd, tx.data(), rx.data(), tx.size());
    std::array<std::uint8_t,N> result{};
    std::copy(rx.begin()+2,rx.end(),result.begin()); return result;
}
#ifndef UKE_TOUCH_TEST
int main() {
    int fd = -1;
    try {
        require(compatible("/sys/firmware/devicetree/base/compatible", "xiaomi,uke"), "Refusing a foreign board");
        require(compatible("/sys/bus/spi/devices/spi0.0/of_node/compatible", "novatek,NVT-ts-spi"), "Refusing a foreign SPI child");
        fd = open("/dev/spidev0.0", O_RDWR | O_CLOEXEC);
        require(fd >= 0, "Reviewed SPI leaf is unavailable");
        lock_touch(fd);
        std::uint8_t mode=255;
        require(ioctl(fd,SPI_IOC_RD_MODE,&mode)==0 && mode==SPI_MODE_0, "Unexpected SPI mode");
        const auto id = read<6>(fd,0x1fb104);
        std::cout << "Trim family bytes: " << std::hex << unsigned(id[3]) << ' '
                  << unsigned(id[4]) << ' ' << unsigned(id[5]) << std::dec << '\n';
        require(nt36532(id), "NT36532 trim identity did not match; no reset attempted");
        const auto topology = read<1>(fd,0x1fb12c)[0];
        std::cout << "NT36532 topology: " << ((topology & 1) ? "single" : "cascade") << '\n';
        firmware(read<16>(fd,firmware_address(topology)));
        close(fd); return 0;
    } catch (const std::exception &e) {
        if(fd >= 0) close(fd);
        std::cerr << e.what() << '\n'; return 1;
    }
}
#endif
