// SPDX-License-Identifier: MIT
#pragma once
#include "installation-policy.hpp"
namespace uke_install {
enum class SelectionKind { unchanged, trial, cancel_trial, return_to_good, rollback };
struct SelectionPlan { SelectionKind kind; Slot target; Update state; };
template<class Save,class Commit,class Select,class Verify,class Restore>
inline void commit_selection(Save save, Commit commit, Select select, Verify verify, Restore restore) {
    try { save(); commit(); select(); verify(); }
    catch(...) {
        try { restore(); }
        catch(...) {throw std::runtime_error("Slot selection and restoration failed; preserve transaction metadata and use recovery before rebooting");}
        throw std::runtime_error("Slot selection failed; original GPT, UFS selection and approval record were restored");
    }
}
inline SelectionPlan select_existing(const Update &saved, Slot running, Slot selected,
                                     Slot target, const std::string &running_release,
                                     const std::string &target_release, bool awaiting_reboot) {
    userdata(saved);
    require(release_token(target_release), "Target kernel release is invalid");
    require(saved.phase == Phase::stable || saved.phase == Phase::trial,
            "Finish or recover the existing kernel transition before selecting a slot");
    const bool good_running = running == saved.good && running_release == saved.good_release;
    const bool trial_running = saved.phase == Phase::trial && running == opposite(saved.good) && running_release == saved.candidate;
    require(good_running || trial_running, "Running kernel does not match the recorded good kernel or trial");
    if (saved.phase == Phase::stable) require(selected == running, "Stable approval and next boot selection disagree");
    else require(selected == saved.good || selected == opposite(saved.good), "Unknown trial selection");
    if (target == selected) {
        require(target_release == (target == saved.good ? saved.good_release : saved.candidate),
                "Selected slot no longer contains the reviewed kernel");
        return {SelectionKind::unchanged, target, saved};
    }
    if (saved.phase == Phase::stable) {
        require(good_running && target == opposite(running), "Only the other verified slot can start a trial");
        auto next = saved;
        (void)stage(next, running, running_release, target_release);
        verified(next); (void)arm(next);
        return {SelectionKind::trial, target, next};
    }
    require(target == saved.good && target_release == saved.good_release,
            "A pending trial can only be retained or returned to its recorded accepted slot");
    auto next = saved;
    if (good_running && awaiting_reboot) {
        next.phase = Phase::stable; next.candidate.clear();
        return {SelectionKind::cancel_trial, target, next};
    }
    if (good_running) {
        returned(next, running, running_release);
        return {SelectionKind::return_to_good, target, next};
    }
    return {SelectionKind::rollback, target, next};
}
} // namespace uke_install
