// SPDX-License-Identifier: MIT
#pragma once

// Explicit development operation only. This never accepts the running trial
// or changes the recorded baseline, and does not qualify that baseline's modules.
static UserdataUpdate replacement_trial(UserdataUpdate saved,RollbackMode mode,unsigned running,
        const std::string &release,const std::string &running_hash,const std::string &retained_hash,
        const std::string &next,const std::string &next_hash,const std::string &boot_id) {
    (void)encode(saved);
    need(mode==RollbackMode::off && running==0 && saved.policy.good==uke_install::Slot::b &&
         saved.policy.phase==uke_install::Phase::trial,"Replacement requires a rollback-off A trial with a recorded B baseline");
    need(release.ends_with("-unstable") && release==saved.policy.candidate && running_hash==saved.candidate_image &&
         retained_hash==saved.good_image,"Running trial or retained B image differs from the protected record");
    need(valid_boot_id(boot_id) && boot_id!=saved.staged_boot_id,"Replacement requires a booted trial");
    need(token(next) && next.ends_with("-unstable") && next!=release && next!=saved.policy.failed &&
         next!=saved.policy.good_release && hex(next_hash) && next_hash!=running_hash && next_hash!=retained_hash,
         "A distinct, nonfailed unstable candidate is required");
    saved.policy.candidate=next;saved.policy.phase=uke_install::Phase::staging;
    saved.candidate_image=next_hash;saved.staged_boot_id=boot_id;
    (void)encode(saved);return saved;
}

// The caller retains the original image only in RAM. Keep the target quarantined
// during image restoration, and never restore bootable metadata after a failed
// image restoration. All callbacks are also exercised by remote fault injection.
template<class Quarantine,class Stage,class Write,class Verify,class Arm,class Finish,
         class Reblock,class RestoreImage,class RestoreMetadata,class RestoreState>
static void replace_trial_transaction(Quarantine quarantine,Stage stage,Write write,Verify verify,Arm arm,
        Finish finish,Reblock reblock,RestoreImage restore_image,RestoreMetadata restore_metadata,RestoreState restore_state) {
    bool attempted_write=false;
    try {
        quarantine();stage();attempted_write=true;write();verify();arm();verify();finish();
    } catch(...) {
        try {
            if(attempted_write) {reblock();restore_image();}
            restore_metadata();restore_state();
        } catch(...) {throw std::runtime_error("Development trial restoration failed; do not reboot before using recovery");}
        throw std::runtime_error("Development trial replacement failed; original image, metadata and protected state restored");
    }
}
