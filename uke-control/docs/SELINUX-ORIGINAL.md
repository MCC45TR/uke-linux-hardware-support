# Original Fedora SELinux policy

Control15 carries a demand-only RPM integration helper for the kernel security
ABI. It uses standard CIL definitions and existing grants from the linked
Fedora policy. The original priority100 policy modules remain unchanged.
There is no service, timer, polling loop or additional runtime package.

The helper reads class definitions from the original base and derives subjects
from the linked policy, preserving conditional grants and disabled modules.
It grants no new BPF or memfd access and changes no policy capability. Once
Fedora supplies all required definitions, the managed compatibility module is
removed automatically. Foreign modules with the same name are refused.

RPM transaction triggers capture the pre-update linked policy before payload
replacement, stage compatibility before standard policy scriptlets and finish
after policy producers. Both staging passes use no-reload libsemanage
operations. Finalization rebuilds the store and reloads only the selected
live-root profile, preserving the enforcement setting. Alternate roots are
always no-reload. Snapshots under /run are temporary and contain no hardware
calibration. No priority100 payload is patched or repackaged.

The supported rollback restores the recorded Core23 and historical policy
cohort while Control15 is still installed. Its helper then removes redundant
compatibility definitions. Control14/TUI14 can be restored afterward. Core24
requires the new integration capability to prevent a partial Control downgrade
while the original Fedora policy still needs the compatibility definitions.

Offline source, representation and module-store results are recorded in
uke-boot/host/SELINUX-ORIGINAL.md. Signed RPM, actual DNF lifecycle, active-policy
and device checks remain separate acceptance gates until explicitly recorded.
