// SPDX-License-Identifier: MIT
// Uses the existing guarded GPT/UFS transaction. Boot image bytes are never written.
#pragma once
#include "slot-selection.hpp"
static void select_existing_slot(const std::string &requested, const std::string &expected_release, bool apply) {
    need(requested=="a" || requested=="b" || requested=="next", "Slot must be a, b or next");
    need(token(expected_release), "Reviewed target kernel release is required");
    const fs::path state="/var/lib/senemos/kernel-update";
    auto state_dir=directory(state);
    Fd lock(openat(state_dir.value,"lock",O_RDWR|O_NOFOLLOW|O_CLOEXEC));trusted_fd(lock.value);
    need(!flock(lock.value,LOCK_EX|LOCK_NB), "Another kernel update is active");
    const auto a=partition("boot_a"), b=partition("boot_b"), root=partition("userdata");
    const auto filesystem=root_mount(root.second,read_text("/proc/self/mountinfo"),"userdata");
    const auto controller=ufs_controller(fs::path("/sys/class/block")/root.first);
    need(ufs_controller(fs::path("/sys/class/block")/a.first)==controller &&
         ufs_controller(fs::path("/sys/class/block")/b.first)==controller, "Boot slots and userdata must share the Uke UFS controller");
    std::array<Bytes,2> images;
    for(unsigned i=0;i<2;++i) { auto fd=physical_boot(i?b:a); images[i]=read_fd(fd.value,capacity); }
    utsname current{};need(!uname(&current) && token(current.release), "Running kernel identity unavailable");
    const auto running_image=file(fs::path("/usr/lib/modules")/current.release/"vmlinuz");
    const auto running=identify_slot(read_text("/proc/cmdline"),images,running_image);
    auto bsg=ufs_bsg(controller);const auto selected=slot::boot_lun(bsg.value)-1;
    const unsigned target=requested=="next"?1-running:requested=="b"?1:0;
    const auto prior=decode(file(state/"userdata.state"));
    need(digest(images[unsigned(prior.policy.good)])==prior.good_image, "Accepted rollback image differs from its recorded identity");
    if(prior.policy.phase==uke_install::Phase::trial) {
        need(digest(images[1-unsigned(prior.policy.good)])==prior.candidate_image, "Pending candidate image differs from its record");
        if(running!=unsigned(prior.policy.good)) {
            auto boot_id=read_text("/proc/sys/kernel/random/boot_id");need(boot_id.ends_with('\n'),"Missing boot identity");boot_id.pop_back();
            need(booted_ab_trial(prior,running,current.release,digest(images[running]),boot_id), "Trial boot identity is inconsistent");
        }
    }
    auto boot_id=read_text("/proc/sys/kernel/random/boot_id");need(boot_id.ends_with('\n'),"Missing boot identity");boot_id.pop_back();
    const bool awaiting=awaiting_ab_reboot(prior,running,current.release,digest(images[running]),boot_id);
    const auto plan=uke_install::select_existing(prior.policy,static_cast<uke_install::Slot>(running),
        static_cast<uke_install::Slot>(selected),static_cast<uke_install::Slot>(target),current.release,expected_release,awaiting);
    auto disks=metadata(controller,false);slot::Disk *boot_table=nullptr;
    for(auto &disk:disks) {
        disk->gpt->require_selected(selected);
        if(disk->gpt->parts.contains("boot_a") && disk->gpt->parts.contains("boot_b")) boot_table=disk->gpt.get();
    }
    need(boot_table, "Boot GPT is unavailable");
    const auto good_flags=boot_table->parts.at(unsigned(prior.policy.good)?"boot_b":"boot_a").attrs;
    need((good_flags&slot::successful) && !(good_flags&slot::unbootable), "The rollback slot must retain its successful-boot flag");
    if(plan.kind==uke_install::SelectionKind::unchanged) {
        std::cout<<"Slot "<<(target?'B':'A')<<" is already selected for the next boot ("<<expected_release<<"). No slot change or reboot.\n";return;
    }
    std::cout<<"Verifying installed target kernel, modules, accepted rollback and early firmware…\n"<<std::flush;
    const auto bundle=fs::path("/usr/lib/senemos/kernel-updates")/expected_release;
    const auto package=validate_bundle(bundle,"/usr/lib/modules","userdata",current.release);
    const auto raw=file(bundle/"manifest");const auto manifest=properties(std::string(raw.begin(),raw.end()));
    need(manifest.at("kernel_release")==expected_release &&
         boot_kernel(images[target])==boot_kernel(package), "Target slot does not match the reviewed installed kernel");
    const auto expected=userdata_vrr(package,vrr_policy(),manifest,filesystem);
    need(images[target]==expected, "Target boot wrapper does not match the current userdata policy; install its admitted bundle first");
    verify_early_firmware();
    UserdataUpdate next=prior;next.policy=plan.state;
    if(plan.kind==uke_install::SelectionKind::trial) { next.candidate_image=digest(images[target]);next.staged_boot_id=boot_id; }
    else if(next.policy.phase==uke_install::Phase::stable) { next.candidate_image.clear();next.staged_boot_id.clear(); }
    std::vector<SlotMetadata*> changed;
    for(auto &disk:disks) {
        const bool update=plan.kind==uke_install::SelectionKind::trial?
            disk->gpt->prepare(selected,target):disk->gpt->fallback(selected,target);
        if(update) changed.push_back(disk.get());
    }
    need(!changed.empty(), "No consistent slot metadata transition was found");
    std::cout<<"Running slot: "<<(running?'B':'A')<<" ("<<current.release<<")\n"
             <<"Next boot: slot "<<(target?'B':'A')<<" ("<<expected_release<<")\n"
             <<"Operation: "<<(plan.kind==uke_install::SelectionKind::trial?"start a verified kernel trial":
                 plan.kind==uke_install::SelectionKind::cancel_trial?"cancel the unbooted trial and retain the accepted kernel":"return to the recorded accepted kernel")
             <<"\nBoot image contents remain unchanged. Reboot is a separate action.\n";
    if(!apply) { std::cout<<"Preview completed; no slot metadata or approval record was written.\n";return; }
    need(charging(), "Battery must be at least 50%, or at least 30% with external power");
    need(fs::space(state).available>32*1024*1024, "Insufficient durable metadata space");
    auto inhibitor=inhibit_shutdown();CriticalSignals signals;
    const auto transaction=state/(expected_release+"-"+digest(images[target]))/"manual-selection";
    auto transaction_dir=directory(transaction,true);
    for(auto *disk:changed)disk->gpt->backup(transaction/(disk->name+"-"+digest(disk->gpt->before.entries)+".gpt"));
    uke_install::commit_selection([&] {
        save_update(state/"userdata.state",next);
    },[&] {
        for(auto *disk:changed)disk->gpt->commit();
    },[&] {
        (void)slot::boot_lun(bsg.value,target+1);
    },[&] {
        for(unsigned i=0;i<2;++i) { auto fd=physical_boot(i?b:a);need(read_fd(fd.value,capacity)==images[i],"Boot image changed during slot selection"); }
    },[&] {
        for(auto *disk:changed)disk->gpt->restore();
        (void)slot::boot_lun(bsg.value,selected+1);
        save_update(state/"userdata.state",prior);
    });
    std::cout<<"Selected slot "<<(target?'B':'A')<<" for the next boot. Both boot images are unchanged. Reboot when ready; no reboot was requested.\n";
}
