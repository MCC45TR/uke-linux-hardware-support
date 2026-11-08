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
};

static int critical_runtime_probe(CriticalRuntimeContext &context) noexcept {
    try {
        critical_runtime_event([&]{
                if(context.adsp_crash_event)
                    throw CriticalHealthFailure("critical: ADSP crash event received in this boot");
                critical_platform_health();
            },
            []{return rollback_name(read_rollback_policy());},
            [] {
                // Serialize against DNF/manual selection; state-file rename
                // wakes this monitor if the writer was busy. No retry timer.
                const fs::path state="/var/lib/senemos/kernel-update";
                auto parent=directory(state);
                Fd lock(openat(parent.value,"lock",O_RDWR|O_NOFOLLOW|O_CLOEXEC));trusted_fd(lock.value);
                if(flock(lock.value,LOCK_EX|LOCK_NB)<0) {
                    need(errno==EWOULDBLOCK,"Cannot lock runtime health state");return false;
                }
                if(read_rollback_policy()==RollbackMode::off)return false;
                utsname current{};need(!uname(&current),"Missing runtime kernel identity");
                const fs::path marker=state/"critical-health";
                replace_policy(marker,policy_exists(marker)?policy_text(marker):std::string{},critical_rejection_text(current.release));
                return true;
            },
            [](const std::string &mode,const char *reason,bool authority) {
                utsname current{};need(!uname(&current),"Missing runtime log identity");
                std::cerr<<boot_health_line({"rejected",current.release,mode,reason,!authority,true})<<std::flush;
            },context.off_reported);
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
    // Unrelated settings and the rejection record itself do not wake a probe.
    if(event && event->len && (std::string_view(event->name)=="userdata.state" ||
                              std::string_view(event->name)=="rollback.conf"))
        return critical_runtime_probe(*static_cast<CriticalRuntimeContext *>(userdata));
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
    (void)critical_runtime_probe(context);
    const auto status=sd_event_loop(event.get());
    need(status>=0,"Critical-health event loop failed");
    if(context.failure)std::rethrow_exception(context.failure);
    return status;
}
