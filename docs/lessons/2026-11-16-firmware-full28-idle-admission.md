# UKE-FIRMWARE-IDLE28-20261116: qualify the running source before additive preparation

16 November 2026. Fresh Fedora root on Uke runs7.3.3-28-unstable from source
8eb8316d. Signed Control20 files install, but the firmware9 transaction trigger
refuses missing current-kernel firmware: its exact idle-source map predates
full28. DNF exits1 and automatic kernel staging refuses the incomplete state.
Installed files do not establish a completed transaction or healthy boot.

Firmware9 already parses complete unstable semantic versions. Its source is
COPR11283443, SRPM9fba3e56, recipe0a9a2334. Retain its original archive, version
header, version patch, all build/check phases and trigger behavior. Firmware10
adds only the exact release28/source8eb tuple to production code. A complete
recipe inverse guards against accidentally using the older8 source recipe.

Full28 is the existing cumulative qualified kernel. Its public composition
retains permanent native CDSP ownership, auto-boot=false, secure override=false,
start/stop retention, SIDs0x0c01/0x0c02 and no distributed pogo MCU. Read-only
tablet observation finds exact native CDSP offline, exact pogo unbound and no
FastRPC CDSP endpoint. Admission still requires these runtime conditions every
time. Private stock identity, bounded read-only acquisition, preflight of every
existing family and additive no-replace publication remain unchanged. No DSP
or trusted application starts and no calibration is copied.

The added native fixture exercises the exact28 profile, wrong source and adjacent
release refusals, active-consumer refusal before output, verification-only
refusal, idle publication, complete-family verification without hardware
access, and newer7.3.4-2 selection as an inactive target. The newer version is
not admitted as a running source by this patch. Compile these tests only in
COPR. Signed RPM verification, DNF reconciliation and actual firmware/boot
health acceptance remain pending.

Stock super currently supplies the MCU acquisition path. Do not replace super
with swap/rescue until the qualified generic-data delivery and verification path
no longer depends on stock logical partitions. This helper fix alone does not
establish out-of-box audio, camera support or that storage transition.
