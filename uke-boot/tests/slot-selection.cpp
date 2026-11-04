// SPDX-License-Identifier: MIT
#include "../src/slot-selection.hpp"
#include <cassert>
#include <iostream>
using namespace uke_install;
int main() {
    const Update stable{Profile::userdata,Slot::b,"7.3.1-1500",Phase::stable,{},"7.3.1-6000"};
    auto p=select_existing(stable,Slot::b,Slot::b,Slot::b,"7.3.1-1500","7.3.1-1500",false);
    assert(p.kind==SelectionKind::unchanged && p.state.good==Slot::b);
    p=select_existing(stable,Slot::b,Slot::b,Slot::a,"7.3.1-1500","7.3.1-1600",false);
    assert(p.kind==SelectionKind::trial && p.state.phase==Phase::trial && p.state.candidate=="7.3.1-1600");
    const auto trial=p.state;
    p=select_existing(trial,Slot::b,Slot::a,Slot::a,"7.3.1-1500","7.3.1-1600",true);
    assert(p.kind==SelectionKind::unchanged);
    p=select_existing(trial,Slot::b,Slot::a,Slot::b,"7.3.1-1500","7.3.1-1500",true);
    assert(p.kind==SelectionKind::cancel_trial && p.state.phase==Phase::stable && p.state.candidate.empty());
    assert(p.state.failed=="7.3.1-6000");
    p=select_existing(trial,Slot::b,Slot::a,Slot::b,"7.3.1-1500","7.3.1-1500",false);
    assert(p.kind==SelectionKind::return_to_good && p.state.failed=="7.3.1-1600");
    p=select_existing(trial,Slot::a,Slot::a,Slot::b,"7.3.1-1600","7.3.1-1500",false);
    assert(p.kind==SelectionKind::rollback && p.state.phase==Phase::trial);
    p=select_existing(trial,Slot::a,Slot::b,Slot::b,"7.3.1-1600","7.3.1-1500",false);
    assert(p.kind==SelectionKind::unchanged);
    const auto refuses=[&](Update s,Slot running,Slot selected,Slot target,std::string running_release,std::string target_release) {
        bool rejected=false;try{select_existing(s,running,selected,target,running_release,target_release,false);}catch(...){rejected=true;}assert(rejected);
    };
    refuses(stable,Slot::b,Slot::b,Slot::a,"7.3.1-1500","7.3.1-6000");
    refuses(stable,Slot::b,Slot::b,Slot::a,"7.3.1-1500","7.3.1-1500");
    refuses(stable,Slot::b,Slot::a,Slot::b,"7.3.1-1500","7.3.1-1500");
    refuses(stable,Slot::a,Slot::a,Slot::b,"7.3.1-1400","7.3.1-1500");
    refuses(trial,Slot::a,Slot::a,Slot::b,"7.3.1-1600","7.3.1-9999");
    auto incomplete=stable;incomplete.phase=Phase::staging;incomplete.candidate="7.3.1-1600";
    refuses(incomplete,Slot::b,Slot::b,Slot::a,"7.3.1-1500","7.3.1-1600");
    incomplete.phase=Phase::current_slot_trial;
    refuses(incomplete,Slot::b,Slot::b,Slot::a,"7.3.1-1500","7.3.1-1600");
    for(int failure=0;failure<4;++failure) {
        int record=0,metadata=0,lun=0,step=0;bool restored=false,rejected=false;
        const auto check=[&] { if(step++==failure)throw std::runtime_error("Injected transaction failure"); };
        try {commit_selection([&]{record=1;check();},[&]{metadata=1;check();},[&]{lun=1;check();},check,
            [&]{record=metadata=lun=0;restored=true;});}catch(...){rejected=true;}
        assert(rejected && restored && !record && !metadata && !lun);
    }
    bool recovery=false;
    try {commit_selection([]{throw std::runtime_error("write");},[]{},[]{},[]{},[]{throw std::runtime_error("restore");});}
    catch(const std::runtime_error &e){recovery=std::string(e.what()).find("recovery")!=std::string::npos;}
    assert(recovery);
    std::cout<<"Existing-slot trial, cancellation, rollback, no-op and refusal contracts passed\n";
}
