# Uke RTC provisioning and verification

3 November 2026, boot integration1.8.39. This package pairs the Uke secure RTC
kernel candidate with UTC provisioning, readback and startup health checks.
It retains first-update policy migration and the 1.8.37 rollback window.

The PMIC counter is preserved. The kernel RTC driver reads and writes its
calendar offset through Qualcomm's RTCInfo secure-variable contract, retaining
the reserved bytes. A missing/factory offset is uninitialized, not a valid
1970 calendar. An unavailable firmware application or unsupported callback must
remain an error. No guessed SDAM address or direct PMIC write is used.

The original Chrony unit pulls `uke-rtc-sync.timer` through the existing project
drop-in. On the matching new DT contract only, its service uses the packaged
native helper. The first trusted calibration requires NTP-synchronized UTC.
The helper refuses malformed calendars, clock steps, synchronization loss,
transport errors and incorrect readback. It does not step the system clock.
An already correct RTC avoids another write. Unsynchronized time is deferred.
On the old kernel the helper performs no RTC write.

Once the secure offset has been initialized, the retained counter supplies
elapsed offline time and the kernel restores full UTC on later boots. The disk
floor remains a separate safeguard. A complete loss/reset of the PMIC counter
requires trusted time again; no filesystem timestamp can recreate unknown
elapsed time. Cold-start retention and actual firmware behavior require a
recoverable device test before making a physical success claim.

`uke-install-profile rtc-check` is a root-only, read-only check of the exact PMK
device, calendar and system UTC. `rtc-sync` uses RTC_SET_TIME only after trusted
clock admission and checks readback. The device node permissions are retained.
The helper does not access NVMEM, firmware partitions or arbitrary EFI variables.
Kernel persistence failures are reported as failures, without an alternate
hardware writer or a fabricated successful result.

For a newly selected RTC-capable kernel, the existing boot-health gate also
requires the RTC check. Initial provisioning and the continuous trial-health
window must finish before acceptance. Existing battery, firmware, signature,
root/UFS and rollback guards remain required. Unsupported RTC firmware must
not silently accept the new kernel as healthy. No normal-update reboot is added.

Delivery uses COPR and a normal DNF transaction. Native injected-I/O and unit
syntax fixtures are software evidence; real ioctl persistence, alarms, reboot,
offline startup and complete power-loss behavior have independent test gates.

After booting the paired kernel, inspect `journalctl -b -u uke-rtc-sync.service`
and run `sudo /usr/libexec/senemos-uke/uke-install-profile rtc-check` for a
read-only calendar comparison. An initial deferred result requires NTP UTC;
the timer retries automatically. A missing RTC or firmware error is a failure,
not a reason to enable direct PMIC writes. A correct live readback alone does
not establish retention: warm restart, offline startup, suspend/alarm behavior
and a cold power cycle need separate observations with the same package cohort.
