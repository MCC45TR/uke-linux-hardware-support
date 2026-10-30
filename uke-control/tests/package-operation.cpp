// SPDX-License-Identifier: MIT
// Faults surround the same coordinator callbacks used by production.
#include "package-operation.hpp"
#include <cassert>
#include <iostream>
using namespace uke::packages::operation;
template<class F>void refused(F&& call){bool denied=false;try{call();}catch(const std::exception&){denied=true;}assert(denied);}
int main() {
    const Record initial{std::string(32,'0'),"stable","prepared","7.2.11-fixture",true};
    assert(encode(decode(encode(initial)))==encode(initial));
    refused([&]{decode(encode(initial)+"phase=complete\n");});
    refused([&]{decode(encode(initial).substr(0,20));});
    auto invalid=initial;invalid.kernel="../../boot";refused([&]{encode(invalid);});
    assert(resume("rpm_started")==Resume::review&&resume("unknown")==Resume::review);
    unsigned interrupted=0;
    for(unsigned fault=1;fault<=32;++fault) {
        auto record=initial;std::string durable=encode(record);unsigned point=0,rpm_calls=0,boot_calls=0;
        bool pending=true,selected=false,inject=true;
        const auto edge=[&]{if(inject&&++point==fault)throw std::runtime_error("injected interruption");};
        Actions actions;
        actions.checkpoint=[&](const Record& state){edge();durable=encode(state);edge();};
        actions.repositories=[&]{edge();edge();};
        actions.rpm=[&]{edge();++rpm_calls;edge();};
        actions.boot=[&]{edge();++boot_calls;edge();};
        actions.select=[&]{edge();selected=true;edge();};
        actions.clear=[&]{edge();pending=false;edge();};
        try{apply(record,actions);}catch(const std::exception&){++interrupted;}
        record=decode(durable);inject=false;
        const auto before=rpm_calls;
        if(resume(record.phase)==Resume::review) {
            refused([&]{apply(record,actions);});refused([&]{finish(record,actions);});
            assert(pending&&!selected&&rpm_calls==before&&boot_calls==0);
        } else {
            if(resume(record.phase)==Resume::replan){assert(!rpm_calls);apply(record,actions);assert(rpm_calls==1);}
            else {finish(record,actions);assert(rpm_calls==before);}
            assert(record.phase=="complete"&&selected&&!pending&&decode(durable).phase=="complete");
        }
    }
    assert(interrupted>=20);
    std::cout<<interrupted<<" coordinator interruption boundaries: no blind RPM replay; idempotent boot/selection finalization passed\n";
}
