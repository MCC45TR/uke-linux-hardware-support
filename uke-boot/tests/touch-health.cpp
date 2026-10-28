// SPDX-License-Identifier: MIT
#include "../src/touch-health.hpp"
#include <cassert>
#include <iostream>
namespace fs=std::filesystem;
static void put(const fs::path &path,const std::string &value) {
    fs::create_directories(path.parent_path());std::ofstream(path,std::ios::binary)<<value;
}
template<class Call>static void refuses(Call call) {
    bool refused=false;try{call();}catch(const std::exception&){refused=true;}assert(refused);
}
int main() {
    char pattern[]="/tmp/uke-touch-health-XXXXXX";
    const auto created=mkdtemp(pattern);assert(created);const fs::path root=created;
    const auto node=root/"sys/bus/spi/devices/spi0.0";
    const auto driver=root/"sys/bus/spi/drivers/nt36532-uke-touch";
    const auto input=node/"input/input5";
    const auto token=[](const std::string &value){return value+'\0';};
    try {
        put(root/"sys/firmware/devicetree/base/compatible",token("xiaomi,uke")+token("qcom,sm7675"));
        put(node/"of_node/compatible",token("novatek,nt36532-uke-thp"));
        auto units=uke_touch::health_units(root);
        assert(units.size()==5 && std::find(units.begin(),units.end(),"uke-touchscreen.service")!=units.end());
        put(node/"of_node/compatible",token("novatek,nt36532-uke-touch"));
        refuses([&]{uke_touch::health_units(root);});
        fs::create_directories(driver);fs::create_directory_symlink(driver,node/"driver");
        refuses([&]{uke_touch::health_units(root);});
        put(input/"name","Novatek NT36532 Uke Touchscreen\n");
        put(input/"properties","2\n");put(input/"capabilities/abs","260800000000003\n");
        put(node/"input_stats","packets=2 errors=0 active=0x0 ready=1\n");
        units=uke_touch::health_units(root);
        assert(units.size()==4 && std::find(units.begin(),units.end(),"uke-touchscreen.service")==units.end());
        put(input/"name","POCO Pad X1 Touchscreen\n");
        assert(uke_touch::native_healthy(root));
        put(input/"name","");refuses([&]{uke_touch::native_healthy(root);});
        put(input/"name","POCO Pad X1 Touchscreen\n");
        put(node/"input_stats","packets=0 errors=1 active=0x0 ready=0\n");
        refuses([&]{uke_touch::health_units(root);});
        const auto panel=root/"sys/class/drm/card2-DSI-1";
        put(panel/"status","connected\n");put(panel/"enabled","disabled\n");
        assert(uke_touch::health_units(root).size()==4);
        put(panel/"enabled","enabled\n");refuses([&]{uke_touch::health_units(root);});
        put(panel/"enabled","invalid\n");refuses([&]{uke_touch::health_units(root);});
        put(panel/"enabled","disabled\n");put(panel/"status","disconnected\n");
        refuses([&]{uke_touch::health_units(root);});put(panel/"status","connected\n");
        const auto duplicate=root/"sys/class/drm/card3-DSI-1";
        put(duplicate/"status","connected\n");put(duplicate/"enabled","disabled\n");
        refuses([&]{uke_touch::health_units(root);});fs::remove_all(duplicate);
        put(node/"input_stats","invalid\n");refuses([&]{uke_touch::health_units(root);});
        put(node/"input_stats","packets=2 errors=0 active=0x0 ready=1\n");
        for(const auto &bits:{"0\n","10000000000000000\n","not-hex\n","-1\n"}) {
            put(input/"capabilities/abs",bits);refuses([&]{uke_touch::health_units(root);});
        }
        put(input/"capabilities/abs","260800000000003\n");put(input/"properties","0\n");
        refuses([&]{uke_touch::health_units(root);});put(input/"properties","2\n");
        fs::remove(node/"driver");fs::create_directory_symlink(root/"sys/bus/spi/drivers/foreign",node/"driver");
        refuses([&]{uke_touch::health_units(root);});fs::remove(node/"driver");
        fs::create_directory_symlink(driver,node/"driver");
        put(root/"sys/bus/spi/devices/spi1.0/of_node/compatible",token("novatek,NVT-ts-spi"));
        refuses([&]{uke_touch::health_units(root);});fs::remove_all(root/"sys/bus/spi/devices/spi1.0");
        put(root/"sys/firmware/devicetree/base/compatible",token("foreign"));
        refuses([&]{uke_touch::health_units(root);});
        fs::remove_all(root);
    } catch(...) {fs::remove_all(root);throw;}
    std::cout<<"Native boot health checks binding, MT capabilities, awake readiness and verified panel-off standby; legacy service checks remain: PASS\n";
}
