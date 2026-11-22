// SPDX-License-Identifier: MIT
#pragma once
#include <systemd/sd-device.h>
#include <systemd/sd-event.h>
#include <sys/inotify.h>
#include <exception>

struct CriticalRuntimeContext {
    sd_event *event;
    std::exception_ptr failure;
    bool off_reported=false;
    bool adsp_crash_event=false;
    bool unknown_reported=false;
    std::unique_ptr<Fd> pending_lock{};
};

static int critical_runtime_probe(CriticalRuntimeContext &context) noexcept {
    try {
        critical_runtime_event([&]{critical_platform_health_latched(context.adsp_crash_event);},
            []{return rollback_name(read_rollback_policy());},
            [&] {
                // Serialize against DNF/manual selection; state-file rename
                // or writer close wakes a busy request. Keep our one unheld
                // descriptor open so its own close cannot cause a retry loop.
                const fs::path state="/var/lib/senemos/kernel-update";
                if(!acquire_critical_runtime_lock(state,context.pending_lock))return false;
                if(read_rollback_policy()==RollbackMode::off) {
                    context.pending_lock.reset();return false;
                }
                utsname current{};need(!uname(&current),"Missing runtime kernel identity");
                const fs::path marker=state/"critical-health";
                replace_policy(marker,policy_exists(marker)?policy_text(marker):std::string{},critical_rejection_text(current.release));
                context.pending_lock.reset();
                return true;
            },
            [](const std::string &mode,const char *reason,bool authority,bool critical) {
                utsname current{};need(!uname(&current),"Missing runtime log identity");
                std::cerr<<boot_health_line({critical?"rejected":"unavailable",current.release,mode,reason,!authority,critical})<<std::flush;
            },context.off_reported,&context.unknown_reported);
    }catch(...) {
        context.failure=std::current_exception();
        (void)sd_event_exit(context.event,trial_health_rejected);
    }
    return 0;
}

static int critical_runtime_device(sd_device_monitor *,sd_device *device,void *userdata) noexcept {
    auto &context=*static_cast<CriticalRuntimeContext *>(userdata);
    const char *subsystem=nullptr,*name=nullptr,*event=nullptr;
    if(sd_device_get_subsystem(device,&subsystem)>=0 &&
       sd_device_get_sysattr_value(device,"name",&name)>=0 &&
       sd_device_get_property_value(device,"RPROC_EVENT",&event)>=0 &&
       critical_remoteproc_event(subsystem,name,event))
        context.adsp_crash_event=true;
    return critical_runtime_probe(context);
}
static int critical_runtime_state(sd_event_source *,const inotify_event *event,void *userdata) noexcept {
    auto &context=*static_cast<CriticalRuntimeContext *>(userdata);
    // A read-only updater may release its lock without renaming the state.
    // Observe that close only while a critical request is waiting for it.
    if(event && event->len && critical_state_event(event->name,bool(context.pending_lock)))
        return critical_runtime_probe(context);
    return 0;
}

static int monitor_critical_runtime() {
    sd_event *raw=nullptr;need(sd_event_new(&raw)>=0,"Cannot create critical-health event loop");
    std::unique_ptr<sd_event,decltype(&sd_event_unref)> event(raw,sd_event_unref);
    sd_device_monitor *monitor_raw=nullptr;
    need(sd_device_monitor_new(&monitor_raw)>=0,"Cannot create remoteproc health monitor");
    std::unique_ptr<sd_device_monitor,decltype(&sd_device_monitor_unref)> monitor(monitor_raw,sd_device_monitor_unref);
    CriticalRuntimeContext context{raw,{}};
    for(const auto subsystem:{"remoteproc","devcoredump"})
        need(sd_device_monitor_filter_add_match_subsystem_devtype(monitor.get(),subsystem,nullptr)>=0,
             "Cannot filter remoteproc health events");
    need(sd_device_monitor_attach_event(monitor.get(),event.get())>=0 &&
         sd_device_monitor_start(monitor.get(),critical_runtime_device,&context)>=0,
         "Cannot start remoteproc health events");
    for(const auto path:{"/etc/senemos","/var/lib/senemos/kernel-update"})
        need(sd_event_add_inotify(event.get(),nullptr,path,IN_MOVED_TO|IN_CLOSE_WRITE,
                                 critical_runtime_state,&context)>=0,
             "Cannot observe critical-health policy/state events");
    std::cout<<"Critical runtime monitor armed; blocking on remoteproc/coredump and policy/state events, no timer or DSP polling\n"<<std::flush;
    try {
        for(const auto &entry:fs::directory_iterator("/sys/class/remoteproc")) {
            if(critical_health_value(entry.path()/"name")!="adsp")continue;
            if(!fs::exists(entry.path()/"crash_count"))
                std::cerr<<"Early ADSP crash history unavailable on this kernel; event and state monitoring remain active\n"<<std::flush;
        }
    }catch(const std::exception &) {
        // Diagnostics must not disable the already armed event listener.
        std::cerr<<"ADSP crash history availability is unknown; event monitoring remains active\n"<<std::flush;
    }
    (void)critical_runtime_probe(context);
    const auto status=sd_event_loop(event.get());
    need(status>=0,"Critical-health event loop failed");
    if(context.failure)std::rethrow_exception(context.failure);
    return status;
}
