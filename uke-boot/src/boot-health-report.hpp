// SPDX-License-Identifier: MIT
#pragma once
#include "trial-health.hpp"
#include "critical-health.hpp"
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

struct BootHealthReport {
    std::string verdict, kernel, mode, detail;
    bool observation_only;
    bool critical = false;
};

inline std::string boot_health_text(std::string_view value) {
    std::string out;
    for(const unsigned char c:value.substr(0,512))
        out+=c<32 || c==127?'?':char(c);
    return out;
}

inline std::string boot_health_line(const BootHealthReport &report) {
    std::ostringstream line;
    // systemd's journal stream recognises the priority prefix. The message is
    // also readable in a terminal and does not imply an approval-state write.
    line<<(report.verdict=="observing"?"<6>":"<4>")
        <<"UKE_KERNEL_HEALTH schema=1 verdict="<<boot_health_text(report.verdict)
        <<" kernel="<<std::quoted(boot_health_text(report.kernel))
        <<" rollback="<<std::quoted(boot_health_text(report.mode))
        <<" approval=pending observation_only="<<(report.observation_only?"yes":"no")
        <<" severity="<<(report.critical?"critical":"ordinary")
        <<" detail="<<std::quoted(boot_health_text(report.detail))<<'\n';
    return line.str();
}

// The existing bounded qualification window is shared by all rollback modes.
// This helper has no approval, metadata, slot-selection or reboot interface.
// A caller outside the acceptance path cannot acquire enforcement by changing
// policy while observation runs. The normal acceptance path re-reads policy
// before propagating a failure, preserving the original mode-change behavior.
template<class Policy,class Probe,class Clock,class Pause,class Emit>
bool observe_boot_health(std::string_view kernel,bool acceptance_path,
                         Policy policy,Probe probe,Clock clock,Pause pause,Emit emit) {
    const auto current_mode=[&] {
        auto mode=policy();
        if(mode!="enforced" && mode!="permissive" && mode!="off")
            throw std::runtime_error("Invalid boot-health rollback mode");
        return mode;
    };
    emit(BootHealthReport{"observing",std::string(kernel),current_mode(),
                         "30 continuous seconds within 90 seconds; approval remains pending",
                         !acceptance_path});
    std::string last_failure;
    try {
        qualify_trial_health([&] {
            try {probe();return true;}
            catch(const CriticalHealthFailure &) {throw;}
            catch(const std::exception &failure) {last_failure=boot_health_text(failure.what());return false;}
        },clock,pause);
    } catch(const CriticalHealthFailure &failure) {
        const auto mode=current_mode();
        emit(BootHealthReport{"rejected",std::string(kernel),mode,
                             boot_health_text(failure.what()),!acceptance_path,true});
        if(acceptance_path && critical_fallback_policy(mode,true))throw;
        return false;
    } catch(const std::exception &failure) {
        const auto mode=current_mode();
        auto reason=std::string(failure.what());
        if(!last_failure.empty())reason+="; last failed check: "+last_failure;
        emit(BootHealthReport{"rejected",std::string(kernel),mode,reason,!acceptance_path});
        if(acceptance_path && mode=="enforced")throw;
        return false;
    }
    emit(BootHealthReport{"healthy-unapproved",std::string(kernel),current_mode(),
                         "Health window passed; kernel approval and slot selection remain separate",
                         !acceptance_path});
    return true;
}
