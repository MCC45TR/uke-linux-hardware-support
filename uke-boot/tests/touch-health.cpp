// SPDX-License-Identifier: MIT
#include "../src/touch-health.hpp"
#include <cassert>
#include <iostream>
#include <unistd.h>
namespace fs=std::filesystem;
static void put(const fs::path &path,const std::string &value) {
    fs::create_directories(path.parent_path());std::ofstream(path,std::ios::binary)<<value;
}
template<class Call>static void refuses(Call call) {
    bool refused=false;try{call();}catch(const std::exception&){refused=true;}assert(refused);
}
static void replace(const fs::path &path,const std::string &value) {
    const auto next=fs::path(path.string()+".next");put(next,value);fs::rename(next,path);
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
        assert(uke_touch::health_units(root,true).size()==5);
        put(node/"of_node/compatible",token("novatek,nt36532-uke-touch"));
        refuses([&]{uke_touch::health_units(root);});
        fs::create_directories(driver);fs::create_directory_symlink(driver,node/"driver");
        refuses([&]{uke_touch::health_units(root);});
        put(input/"name","Novatek NT36532 Uke Touchscreen\n");
        put(input/"properties","2\n");put(input/"capabilities/abs","260800000000003\n");
        put(node/"input_stats","packets=2 errors=0 active=0x0 ready=1\n");
        units=uke_touch::health_units(root);
        assert(units.size()==4 && std::find(units.begin(),units.end(),"uke-touchscreen.service")==units.end());
        assert(uke_touch::health_units(root,true).size()==4);
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
        const auto during_wait=[&](auto change,const std::string &expected_error="") {
            std::thread worker([&]{std::this_thread::sleep_for(std::chrono::milliseconds(250));change();});
            std::string error;std::vector<std::string> result;
            try {result=uke_touch::health_units(root,true);}
            catch(const std::exception &failure){error=failure.what();}
            worker.join();assert(error==expected_error);
            if(error.empty())assert(result.size()==4);
        };
        during_wait([&]{replace(node/"input_stats","packets=32 errors=0 active=0x0 ready=1\n");});
        put(node/"input_stats","packets=0 errors=1 active=0x0 ready=0\n");
        during_wait([&]{replace(panel/"enabled","disabled\n");});
        put(panel/"enabled","enabled\n");
        during_wait([&]{replace(input/"capabilities/abs","invalid\n");},"Native touchscreen capabilities are malformed");
        put(input/"capabilities/abs","260800000000003\n");
        during_wait([&]{replace(node/"of_node/compatible",token("novatek,nt36532-uke-thp"));},"Native touchscreen disappeared during startup polling");
        put(node/"of_node/compatible",token("novatek,nt36532-uke-touch"));
        alarm(10);
        const auto started=std::chrono::steady_clock::now();bool pending=false;
        try {uke_touch::health_units(root,true);}
        catch(const uke_touch::ReadinessPending &){pending=true;}
        alarm(0);assert(pending && std::chrono::steady_clock::now()-started>=std::chrono::seconds(2));
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
        refuses([&]{uke_touch::health_units(root,true);});
        fs::remove_all(root);
    } catch(...) {fs::remove_all(root);throw;}
    std::cout<<"Native touch health: binding, MT capabilities, readiness/panel-off recovery, pending timeout and changing/foreign metadata refusals passed; legacy and immediate checks remain\n";
}
