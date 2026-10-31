// SPDX-License-Identifier: MIT
#include "../src/fedora-target.hpp"
#include <cassert>
using namespace uke::packages;
int main() {
    assert(fedora_target("ID=fedora\nVERSION_ID=44\n")=="44");
    assert(fedora_target("ID=\"fedora\"\nVERSION_ID=\"45\"\n")=="45");
    assert(fedora_target("ID=fedora\nVERSION_ID=46\nVERSION=\"Rawhide\"\n")=="rawhide");
    assert(fedora_target("ID=fedora\nVERSION_ID=rawhide\n")=="rawhide");
    assert(fedora_target("ID=fedora\nVERSION_ID=46\nVERSION=\"46 (Rawhide Prerelease)\"\nRELEASE_TYPE=development\nREDHAT_BUGZILLA_PRODUCT_VERSION=rawhide\n")=="rawhide");
    for(const auto* text:{"ID=ubuntu\nVERSION_ID=44\n","ID=fedora\nVERSION_ID=43\n","ID=fedora\nVERSION_ID=46\n","ID=fedora\nPRETTY_NAME=Rawhide\n",
        "ID=fedora\nVERSION_ID=46\nVERSION=\"46 (Workstation Edition)\"\nRELEASE_TYPE=stable\n",
        "ID=fedora\nVERSION_ID=46\nVERSION=\"46 (Rawhide Prerelease)\"\nRELEASE_TYPE=development\n",
        "ID=fedora\nVERSION_ID=46\nVERSION=\"47 (Rawhide Prerelease)\"\nRELEASE_TYPE=development\nREDHAT_BUGZILLA_PRODUCT_VERSION=rawhide\n"}) {
        bool refused=false;try{fedora_target(text);}catch(const std::runtime_error&){refused=true;}assert(refused);
    }
    assert(fedora_package_release("1.fc44",".fc44"));
    assert(fedora_package_release("1.fc44.1",".fc44"));
    assert(!fedora_package_release("1.fc45",".fc44"));
    assert(!fedora_package_release("1.fc440",".fc44"));
    assert(!fedora_package_release("1",".fc44"));
}
