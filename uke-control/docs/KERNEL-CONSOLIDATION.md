# Kernel delivery in Control

7 November 2026: the separate installation-profile package's file requirement
can prevent ordinary DNF upgrades when filelists are not loaded. A device dry
run reproduces the failure. Loading filelists resolves all six systemd upgrade
candidates. The original Fedora `systemd-udev` package still owns `systemd-growfs`.

The candidate retains exact source RPMs Control2.8.1-26/COPR11268045 and boot
integration1.8.102-1/COPR11272089. Control's terminal/client/diagnostic payloads
and the boot, installation-profile and kernel-update runtime become one Control
RPM. The existing optional client SDK remains a separate build dependency.
No previously uninstalled development recovery aliases are added to the runtime.
All source archives remain unchanged and all original build and check sections
run in COPR. No native compilation runs on the tablet.

Compatibility names and feature capabilities satisfy existing kernel and GUI
dependencies. Original helpers retain their paths, while `uke-control` remains
the public settings interface. DNF invokes the retained updater through one
transaction file trigger. Alternating A/B, complete module/boot admission,
rollback defaults and the selected local rollback preference remain unchanged.
The three persistent update configuration files retain `config(noreplace)`.

An explicit migration snapshot preserves enabled, disabled, masked, runtime and
active states of units that legacy package-removal scripts can disable. The
emitted lifecycle fragments are tested with isolated providers. No snapshot
contains calibration, keys, audio data or a partition backup. Kernel selection
and raw slot preservation require separate device admission.

The dependency now names the original Fedora provider `systemd-udev`, avoiding
the filelist bootstrap problem when replacing the old classifier package. A
vendor DNF configuration loads filelists for other retained file dependencies;
administrator configuration keeps its normal precedence. Protected-package,
signature and dependency checks remain enabled.

The candidate needs COPR compilation, actual RPM dependency/script/file review,
upgrade-solver checks and a DNF device transaction before runtime acceptance.
This source change does not establish new driver, encrypted-root, Secure Boot,
TEE key-service, speaker-protection or HDMI timing acceptance.
