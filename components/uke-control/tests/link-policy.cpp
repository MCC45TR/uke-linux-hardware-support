// SPDX-License-Identifier: MIT
#include "link-policy.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
namespace fs=std::filesystem;
static void put(const fs::path& p,const std::string& s){fs::create_directories(p.parent_path());std::ofstream(p)<<s;}
template<class F> static void refuses(F f){try{f();assert(false);}catch(const std::runtime_error&){}}
int main() {
    const auto capabilities=[](const std::string& modes,const std::string& advertised="\"1000baseT/Full\"",bool enabled=true) {
        return "[{\"ifname\":\"eth-test\",\"supports-auto-negotiation\":true,\"auto-negotiation\":"+
            std::string(enabled?"true":"false")+",\"supported-link-modes\":["+modes+"],\"advertised-link-modes\":["+advertised+"]}]";
    };
    const std::string full="\"10baseT/Half\",\"10baseT/Full\",\"100baseT/Half\",\"100baseT/Full\",\"1000baseT/Full\"";
    const auto all=capabilities(full);
    for(const auto& [profile,mask]:std::vector<std::pair<std::string,std::string>>{{"auto","2f"},{"1000-full","20"},{"100-full","8"},{"10-full","2"}}) {
        const auto args=uke::links::ethernet_arguments("eth-test",profile,all);
        assert(args==std::vector<std::string>({"--change","eth-test","autoneg","on","advertise",mask+"/3f"}));
        const unsigned requested=std::stoul(mask,nullptr,16);
        for(unsigned flags:{0u,0x2000u,0x4000u,0x6000u,0xffff0000u}) {
            const unsigned before=flags|0x2f;
            const unsigned after=(before&~0x3fu)|(requested&0x3f);
            assert((after&~0x3fu)==(before&~0x3fu));
        }
    }
    assert(uke::links::ethernet_matches("eth-test","1000-full",all));
    assert(!uke::links::ethernet_matches("eth-test","100-full",all));
    assert(!uke::links::ethernet_matches("eth-test","auto",all));
    assert(uke::links::ethernet_matches("eth-test","auto",capabilities(full,full)));
    assert(!uke::links::ethernet_matches("eth-test","1000-full",capabilities(full,"\"1000baseT/Full\"",false)));
    refuses([&]{uke::links::ethernet_arguments("eth-test","1000-full",capabilities("\"100baseT/Full\""));});
    refuses([&]{uke::links::ethernet_arguments("wrong","auto",all);});
    refuses([&]{uke::links::ethernet_arguments("../eth-test","auto",all);});
    refuses([&]{uke::links::ethernet_arguments("eth-test","2000-full",all);});
    for(const auto* bad:{"[]","{}","warning\n[]","[{},{}]","[{\"ifname\":\"eth-test\"}]"})
        refuses([&]{uke::links::ethernet_arguments("eth-test","auto",bad);});
    refuses([&]{uke::links::ethernet_arguments("eth-test","auto",capabilities("\"2500baseT/Full\""));});
    char pattern[]="/tmp/uke-link-policy-XXXXXX";const auto root=fs::path(mkdtemp(pattern));
    try {
        auto controller=root/"sys/devices/platform/soc/a600000.ssusb";
        const auto driver=root/"sys/bus/platform/drivers/dwc3-qcom";fs::create_directories(driver);
        fs::create_directories(controller);fs::create_directory_symlink(driver,controller/"driver");
        auto usb=controller/"a600000.usb/xhci-hcd.0.auto/usb2/2-1";
        put(usb/"idVendor","0bda\n");put(usb/"idProduct","8153\n");put(usb/"devpath","1\n");
        fs::create_directories(root/"sys/bus/usb/devices");fs::create_directory_symlink(usb,root/"sys/bus/usb/devices/2-1");
        assert(uke::links::usb_device(root,"2-1")==usb);
        refuses([&]{uke::links::usb_device(root,"../../etc");});
        put(usb/"devpath","0\n");refuses([&]{uke::links::usb_device(root,"2-1");});put(usb/"devpath","1\n");
        auto iface=usb/"2-1:1.0";auto net=iface/"net/eth-test";
        fs::create_directories(net);fs::create_directory_symlink(iface,net/"device");
        fs::create_directories(root/"sys/bus/usb/drivers/r8152");
        fs::create_directory_symlink(root/"sys/bus/usb/drivers/r8152",iface/"driver");
        fs::create_directories(root/"sys/class/net");fs::create_directory_symlink(net,root/"sys/class/net/eth-test");
        assert(uke::links::ethernet_device(root,"eth-test")==net);
        put(usb/"idProduct","8156\n");refuses([&]{uke::links::ethernet_device(root,"eth-test");});
        fs::remove(controller/"driver");refuses([&]{uke::links::usb_device(root,"2-1");});
        const auto dp=root/"sys/bus/platform/devices/dp";put(dp/"uke_dp_lane_limit","4\n");
        const auto dpdriver=root/"sys/bus/platform/drivers/msm-dp-display";fs::create_directories(dpdriver);
        fs::create_directory_symlink(dpdriver,dp/"driver");fs::create_directory_symlink(dp,dpdriver/"dp");
        assert(uke::links::lane_controls(root)==std::vector<fs::path>{dp/"uke_dp_lane_limit"});
        assert(uke::links::compression_controls(root).empty());
        assert(uke::links::compression_status(root).find("kernel_control_unavailable")!=std::string::npos);
        put(dp/"uke_dp_dsc","0\n");put(dp/"uke_dp_stream_state","version=1 active=0 dsc_active=0\n");
        assert(uke::links::compression_controls(root)==std::vector<fs::path>{dp/"uke_dp_dsc"});
        auto compression=uke::links::compression_status(root);
        assert(compression.find("\"requested\":false")!=compression.npos&&compression.find(root.string())==compression.npos);
        put(dp/"uke_dp_dsc","1\n");
        assert(uke::links::compression_status(root).find("\"requested\":true")!=std::string::npos);
        assert(uke::links::compression_status(root).find("\"requires_disconnected\":true")!=std::string::npos);
        assert(uke::links::compression_value("on",1)=="1");
        assert(uke::links::compression_value("off",1)=="0");
        refuses([&]{uke::links::compression_value("auto",1);});
        refuses([&]{uke::links::compression_value("on",3);});
        refuses([&]{uke::links::compression_value("on;unexpected",2);});
        put(dp/"uke_dp_dsc","auto\n");
        assert(uke::links::compression_status(root).find("invalid_kernel_policy")!=std::string::npos);
        const std::string live="version=1 active=1 dsc_active=0 dsc_control=2 dsc_failed=0\n";
        put(dp/"uke_dp_stream_state",live);
        compression=uke::links::compression_status(root);
        assert(compression.find("\"control_available\":true")!=compression.npos);
        assert(compression.find("\"driver_interface_version\":2")!=compression.npos);
        assert(compression.find("\"policy\":\"auto\"")!=compression.npos);
        assert(compression.find("\"requested\":true")!=compression.npos);
        assert(compression.find("\"forced\":false")!=compression.npos);
        assert(compression.find("\"dsc_active\":false")!=compression.npos);
        assert(compression.find("\"requires_disconnected\":false")!=compression.npos);
        assert(compression.find("\"live_change_supported\":true")!=compression.npos);
        assert(uke::links::compression_value("auto",2)=="auto");
        assert(uke::links::compression_value("on",2)=="1");
        assert(uke::links::compression_value("off",2)=="0");
        put(dp/"uke_dp_dsc","1\n");
        put(dp/"uke_dp_stream_state","version=1 active=1 dsc_active=1 dsc_control=2\n");
        compression=uke::links::compression_status(root);
        assert(compression.find("\"forced\":true")!=compression.npos);
        assert(compression.find("\"dsc_active\":true")!=compression.npos);
        // A retained request can coexist with plain output after receiver failure.
        put(dp/"uke_dp_stream_state","version=1 active=1 dsc_active=0 dsc_control=2 dsc_failed=1 dsc_fallbacks=1\n");
        compression=uke::links::compression_status(root);
        assert(compression.find("\"requested\":true")!=compression.npos);
        assert(compression.find("\"dsc_active\":false")!=compression.npos);
        for(const auto* bad:{"version=2 active=1 dsc_control=2", "version=1 dsc_control=3",
                "version=1 dsc_control=-1", "version=1 dsc_control=2junk", "version=1 dsc_control=2 dsc_control=2",
                "version=1 version=1 dsc_control=2"}) {
            put(dp/"uke_dp_stream_state",bad);
            assert(uke::links::compression_status(root).find("unsupported_driver_interface")!=std::string::npos);
        }
        for(const auto* uncertain:{"version=1 dsc_control=2", "version=1 active=1 dsc_active=2 dsc_control=2",
                "version=1 active=0 dsc_active=1 dsc_control=2", "version=1 active=1 dsc_active=1 dsc_active=1 dsc_control=2"}) {
            put(dp/"uke_dp_stream_state",uncertain);
            assert(uke::links::compression_status(root).find("\"dsc_active\":null")!=std::string::npos);
        }
        put(dp/"uke_dp_stream_state",live);
        put(dp/"uke_dp_dsc","2\n");
        assert(uke::links::compression_status(root).find("invalid_kernel_policy")!=std::string::npos);
        const auto dp2=root/"sys/bus/platform/devices/dp2";put(dp2/"uke_dp_dsc","0\n");
        fs::create_directory_symlink(dpdriver,dp2/"driver");fs::create_directory_symlink(dp2,dpdriver/"dp2");
        assert(uke::links::compression_status(root).find("ambiguous_kernel_control")!=std::string::npos);
        fs::remove(dpdriver/"dp2");fs::remove_all(dp2);
        fs::remove(dp/"uke_dp_dsc");fs::create_symlink("/etc/passwd",dp/"uke_dp_dsc");
        assert(uke::links::compression_controls(root).empty());
        refuses([&]{uke::links::compression("on;unexpected");});
        fs::remove(dp/"uke_dp_lane_limit");fs::create_symlink("/etc/passwd",dp/"uke_dp_lane_limit");
        assert(uke::links::lane_controls(root).empty());
        const auto output=root/"sys/class/drm/card2-DP-1";
        const auto bits=root/"sys/kernel/debug/dri/2/DP-1/output_bits";
        put(output/"status","connected\n");put(output/"enabled","enabled\n");
        put(output/"max_bpc","16\n");
        auto status=uke::links::display_bits_status(root);
        assert(status.find("\"bits_per_component\":null")!=status.npos);
        put(bits,"version=1 active=1 bpc=8 bpp=24 dsc_bpp_x16=0\n");
        status=uke::links::display_bits_status(root);
        assert(status.find("\"bits_per_component\":8")!=status.npos);
        assert(status.find("\"bits_per_pixel\":24")!=status.npos);
        for(const auto* bad:{"", "version=2 active=1 bpc=10 bpp=30 dsc_bpp_x16=0",
                "version=1 active=1 bpc=10 bpp=24 dsc_bpp_x16=0",
                "version=1 active=0 bpc=8 bpp=24 dsc_bpp_x16=0",
                "version=1 active=1 bpc=999999999999999999 bpp=30 dsc_bpp_x16=0",
                "version=1 active=1 bpc=10 bpp=30 dsc_bpp_x16=999",
                "version=1 active=1 bpc=10 bpp=30 dsc_bpp_x16=0 extra=1"}) {
            put(bits,bad);status=uke::links::display_bits_status(root);
            assert(status.find("invalid_driver_snapshot")!=status.npos);
            assert(status.find("\"bits_per_component\":null")!=status.npos);
        }
        put(bits,"version=1 active=1 bpc=10 bpp=30 dsc_bpp_x16=128\n");
        status=uke::links::display_bits_status(root);
        assert(status.find("\"bits_per_component\":10")!=status.npos);
        assert(status.find("\"dsc_bits_per_pixel_x16\":128")!=status.npos);
        put(output/"status","disconnected\n");status=uke::links::display_bits_status(root);
        assert(status.find("\"active\":false")!=status.npos);
        assert(status.find("\"bits_per_component\":null")!=status.npos);
        put(output/"status","connected\n");put(output/"enabled","disabled\n");
        status=uke::links::display_bits_status(root);
        assert(status.find("\"bits_per_component\":null")!=status.npos);
        put(output/"enabled","enabled\n");put(bits,"version=1 active=0 bpc=0 bpp=0 dsc_bpp_x16=0\n");
        status=uke::links::display_bits_status(root);
        assert(status.find("\"active\":false")!=status.npos);
        fs::remove(bits);fs::create_symlink("/etc/passwd",bits);
        status=uke::links::display_bits_status(root);
        assert(status.find("snapshot_unavailable")!=status.npos);
        fs::remove_all(root);
    }catch(...){fs::remove_all(root);throw;}
    std::cout<<"Link policy: interface/USB topology scope, sysfs containment, speed support, JSON errors and advertisement readback passed; host only\n";
}
