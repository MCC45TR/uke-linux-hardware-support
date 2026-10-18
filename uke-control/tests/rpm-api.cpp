// SPDX-License-Identifier: MIT
#include "rpm-api.hpp"
#include <cassert>
int main() {
    constexpr auto name="uke-usb-manual-cdc-api";
    assert(uke::rpm_api_matches("package\t0.1.0-20.fc46\nuke-usb-manual-cdc-api\t2\n",name,"2"));
    for(auto text : {"", "uke-usb-manual-cdc-api = 2\n", "uke-usb-manual-cdc-api\t1\n",
                    "uke-usb-manual-cdc-api\t21\n", "uke-usb-manual-cdc-api\t2",
                    "uke-usb-manual-cdc-api\t2 \n", "uke-usb-manual-cdc-api\t2\r\n",
                    "uke-usb-manual-cdc-api\t2\nuke-usb-manual-cdc-api\t2\n",
                    "uke-usb-manual-cdc-api\t2\nuke-usb-manual-cdc-api\t1\n"})
        assert(!uke::rpm_api_matches(text,name,"2"));
    assert(!uke::rpm_api_matches("uke-usb-gadget-ownership-api\t2\n",name,"2"));
}
