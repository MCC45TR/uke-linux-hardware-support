// SPDX-License-Identifier: MIT
#include "../src/fedora-target.hpp"
#include <cassert>
using namespace uke::packages;
int main() {
    assert(fedora_target("ID=fedora\nVERSION_ID=44\n")=="44");
    assert(fedora_target("ID=\"fedora\"\nVERSION_ID=\"45\"\n")=="45");
    assert(fedora_target("ID=fedora\nVERSION_ID=46\nVERSION=\"Rawhide\"\n")=="rawhide");
    assert(fedora_target("ID=fedora\nVERSION_ID=rawhide\n")=="rawhide");
    assert(fedora_target("ID=fedora\nVERSION_ID=46\nVERSION=\"46 (Rawhide Prerelease)\"\n")=="rawhide");
    assert(fedora_target("ID=fedora\nVERSION_ID=47\nVERSION=\"47 (Rawhide)\"\n")=="rawhide");
    const auto rawhide=fedora_release("ID=fedora\nVERSION_ID=46\nVERSION=\"46 (Rawhide Prerelease)\"\n");
    assert(fedora_dist(rawhide,"rawhide")==".fc46");
    assert(fedora_dist(rawhide,"46")==".fc46");
    assert(fedora_dist(fedora_release("ID=fedora\nVERSION_ID=44\n"),"44")==".fc44");
    assert(fedora_dist(fedora_release("ID=fedora\nVERSION_ID=rawhide\n"),"46")==".fc46");
    for(const auto* releasever:{"45","","46beta"}) {
        bool refused=false;try{fedora_dist(rawhide,releasever);}catch(const std::runtime_error&){refused=true;}assert(refused);
    }
    for(const auto* releasever:{"rawhide","45"}) {
        bool refused=false;try{fedora_dist(fedora_release("ID=fedora\nVERSION_ID=44\n"),releasever);}catch(const std::runtime_error&){refused=true;}assert(refused);
    }
    bool unresolved=false;try{fedora_dist(fedora_release("ID=fedora\nVERSION_ID=rawhide\n"),"rawhide");}catch(const std::runtime_error&){unresolved=true;}assert(unresolved);
    for(const auto* text:{"ID=fedora\nVERSION_ID=46\nVERSION=46\n","ID=fedora\nVERSION_ID=46\nVERSION=\"46 (Beta)\"\n","ID=fedora\nVERSION_ID=46\nVERSION=\"47 (Rawhide)\"\n"}) {
        bool refused=false;try{fedora_target(text);}catch(const std::runtime_error&){refused=true;}assert(refused);
    }
    for(const auto* text:{"ID=ubuntu\nVERSION_ID=44\n","ID=fedora\nVERSION_ID=43\n","ID=fedora\nVERSION_ID=46\n","ID=fedora\nPRETTY_NAME=Rawhide\n","ID=fedora\nVERSION_ID=46beta\nVERSION=Rawhide\n"}) {
        bool refused=false;try{fedora_target(text);}catch(const std::runtime_error&){refused=true;}assert(refused);
    }
    assert(fedora_package_release("1.fc44",".fc44"));
    assert(fedora_package_release("1.fc44.1",".fc44"));
    assert(!fedora_package_release("1.fc45",".fc44"));
    assert(!fedora_package_release("1.fc440",".fc44"));
    assert(!fedora_package_release("1",".fc44"));
}
