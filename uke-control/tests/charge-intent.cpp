// SPDX-License-Identifier: MIT
#include "charge-intent.hpp"
#include <cassert>
int main() {
    using namespace uke::charge::intent;
    assert(restore_legacy("",90));assert(restore_legacy(standard(90),90));
    for(const auto* mode:{"charge80","hold70-80","hold80-90","slow","automatic","profile-restore","enable-cap80","apply-cap80"})assert(!restore_legacy(qualified(mode),90));
    for(const auto& record:{standard(80),std::string("format=2\n"),std::string("format=1\nprovider=qualified\nrequested=unknown\n")}) {
        bool refused=false;try{restore_legacy(record,90);}catch(const std::exception&){refused=true;}assert(refused);
    }
    for(const auto* mode:{"firmware","temporary-full","unknown"}){bool refused=false;try{qualified(mode);}catch(const std::exception&){refused=true;}assert(refused);}
}
