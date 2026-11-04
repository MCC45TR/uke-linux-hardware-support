// SPDX-License-Identifier: MIT
#pragma once
#include "local.hpp"
namespace uke_local_fw {
static inline bool path_present(const fs::path &path) {
    std::error_code error;const auto status=fs::symlink_status(path,error);
    need(!error||error==std::errc::no_such_file_or_directory,"Cannot establish firmware consumer path state");
    return status.type()!=fs::file_type::not_found;
}
template<class ReadSysfs> void require_idle_consumers(const fs::path &sys,const fs::path &dev,ReadSysfs read) {
    const auto cdsp_node=sys/"firmware/devicetree/base/soc/remoteproc-cdsp@32300000";
    const auto pogo_node=sys/"firmware/devicetree/base/soc/qcom,qupv3_0_geni_se@ac0000/i2c@a8c000/nanosic@4c";
    const auto expected_driver=sys/"bus/platform/drivers/qcom_cliffs_cdsp_pas";
    unsigned cdsp=0,pogo=0;
    for(const auto &entry:fs::directory_iterator(sys/"class/remoteproc")) {
        std::error_code error;const auto node=fs::canonical(entry.path()/"device/of_node",error);
        if(error||node!=cdsp_node)continue;
        need(++cdsp==1,"Duplicate CDSP consumer identity");
        need(fs::canonical(entry.path()/"device/driver")==expected_driver,"Unexpected CDSP driver");
        need(read(entry.path()/"state")=="offline\n","CDSP must be offline before additive firmware preparation");
    }
    need(cdsp==1,"The exact offline native CDSP consumer is required");
    for(const auto &entry:fs::directory_iterator(sys/"bus/i2c/devices")) {
        std::error_code error;const auto node=fs::canonical(entry.path()/"of_node",error);
        if(error||node!=pogo_node)continue;
        need(++pogo==1,"Duplicate pogo consumer identity");
        need(!path_present(entry.path()/"driver"),"Pogo must be unbound before additive firmware preparation");
    }
    need(pogo==1,"The exact unbound Uke pogo consumer is required");
    need(!path_present(dev/"fastrpc-cdsp")&&!path_present(dev/"fastrpc-cdsp-secure"),
         "An active FastRPC CDSP endpoint prevents current-kernel preparation");
}
}
