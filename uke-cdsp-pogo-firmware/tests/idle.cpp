// SPDX-License-Identifier: MIT
#define UKE_LOCAL_FW_TEST
#define UKE_KERNEL_UPDATE_TEST
#include "../src/idle-current.hpp"
#include <cassert>
#include <fstream>
using namespace uke_local_fw;
template<class F> static void refused(F f) {
    bool failed=false;try{f();}catch(const std::exception&){failed=true;}assert(failed);
}
static void text_file(const fs::path &path,const std::string &value) {
    std::ofstream output(path);output<<value;output.close();assert(output.good());
}
int main() {
    char pattern[]="/tmp/uke-idle-contract-XXXXXX";const auto made=mkdtemp(pattern);assert(made);
    const fs::path root=made,sys=root/"sys",dev=root/"dev";
    const auto cdsp_node=sys/"firmware/devicetree/base/soc/remoteproc-cdsp@32300000";
    const auto pogo_node=sys/"firmware/devicetree/base/soc/qcom,qupv3_0_geni_se@ac0000/i2c@a8c000/nanosic@4c";
    const auto driver=sys/"bus/platform/drivers/qcom_cliffs_cdsp_pas";
    const auto platform=sys/"devices/platform/cdsp",remote=sys/"class/remoteproc/remoteproc0";
    const auto pogo=sys/"bus/i2c/devices/5-004c";
    for(const auto &path:{cdsp_node,pogo_node,driver,platform,remote,pogo,dev})fs::create_directories(path);
    fs::create_directory_symlink(cdsp_node,platform/"of_node");
    fs::create_directory_symlink(driver,platform/"driver");
    fs::create_directory_symlink(platform,remote/"device");
    fs::create_directory_symlink(pogo_node,pogo/"of_node");
    text_file(remote/"state","offline\n");
    const auto read=[](const auto &path){const auto bytes=file(path);return std::string(bytes.begin(),bytes.end());};
    const auto check=[&]{require_idle_consumers(sys,dev,read);};
    check();
    // Unrelated active remote processors are retained, not required to stop.
    fs::create_directories(sys/"class/remoteproc/remoteproc1");
    text_file(sys/"class/remoteproc/remoteproc1/state","running\n");check();
    for(const auto &state:{"running\n","crashed\n","offline\nextra","offline"}) {
        text_file(remote/"state",state);refused(check);
    }
    text_file(remote/"state","offline\n");
    fs::create_directory_symlink(driver,pogo/"driver");refused(check);fs::remove(pogo/"driver");
    // Even a dangling driver link cannot masquerade as an unbound device.
    fs::create_directory_symlink(root/"absent",pogo/"driver");refused(check);fs::remove(pogo/"driver");
    for(const auto &node:{"fastrpc-cdsp","fastrpc-cdsp-secure"}) {
        text_file(dev/node,"fixture");refused(check);fs::remove(dev/node);
    }
    fs::remove(platform/"driver");fs::create_directory_symlink(root,platform/"driver");
    refused(check);fs::remove(platform/"driver");fs::create_directory_symlink(driver,platform/"driver");
    const auto duplicate=sys/"class/remoteproc/remoteproc2";fs::create_directories(duplicate);
    fs::create_directory_symlink(platform,duplicate/"device");text_file(duplicate/"state","offline\n");
    refused(check);fs::remove_all(duplicate);
    fs::remove(pogo/"of_node");fs::create_directory_symlink(cdsp_node,pogo/"of_node");refused(check);
    fs::remove(pogo/"of_node");fs::create_directory_symlink(pogo_node,pogo/"of_node");
    fs::remove(platform/"of_node");refused(check);fs::create_directory_symlink(cdsp_node,platform/"of_node");
    check();assert(!fs::exists(root/"firmware"));
    fs::remove_all(root);
    std::cout<<"PASS: actual idle-consumer discovery, unrelated active devices, driver/node/duplicate/state refusals; synthetic filesystem only\n";
}
