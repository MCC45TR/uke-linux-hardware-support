# Separate the DSP filesystem service from sensor presentation

6 November 2026, POCO Pad X1, Fedora AArch64. The existing native IIO consumer
owns sensor QMI transport and channel publication. A direct-read snapshot on
kernel 12206 successfully reads all seven native devices and their standard
attributes; the acceleration magnitude is about 9.76 m/s². This is a transport
and unit sanity observation, without pose, accuracy or lifecycle acceptance.

The installed filesystem service comes from complete source family
`uke-sensor-integration-1.0.0-3`, COPR 11239521. Its native daemon links only
libsystemd, libc and the ELF loader. All three required firmware/configuration paths belong to
`uke-firmware`; its service units use FastRPC and the private RAM registry,
without calling SensorProxy. The two SSC/dual-light SensorProxy capability
requirements therefore impose an unrelated presentation dependency.

Release 32 removes those two requirements while preserving all native sources,
firmware file dependencies, read-only persist admission, RAM registry, units,
ordering, scripts and original native tests. The full parent source capsule and
its seal are verified. Existing source catalogues remain historical inputs;
the explicit service-decoupling manifest defines this update's output selection.
Completed front-color/proximity observation clients are not rebuilt as new RPM
outputs; their retained source tests still run. Matching service debug packages
remain available. No firmware, factory calibration or unique identity is added.

This does not remove the installed presentation frontend. Core meta and
adaptive-display still require it. Replacement with the original distribution
SensorProxy requires axes, both light paths, proximity, startup/restart/suspend,
dependency closure and preserved user settings. Keep the FastRPC filesystem
service: native IIO still depends on the firmware's file protocol. Kernel/DT
cannot replace that protocol with private filesystem parsing. No KDE/GNOME
package is forked or rebuilt by this change, and no local device compilation
or copied executable is used.

Initial source job 11268448 refuses before producing an SRPM: rpmspec lists
the source-only main header alongside binary packages, and the output guard
incorrectly treated it as an extra runtime output. The corrected guard admits
that metadata header while requiring exactly the three service %files sections.
Fresh source release 5 and service release 31 preserve the same dependency-only
change. Native build, signed payload and actual output admission remain pending.

Source job 11268497 then passes RPM parsing but the file-section guard excludes
the runtime package whose declaration ends at the package name. Replace that
regular expression with exact parsed section-name comparison against the three
allowed packages. The real parent transformation passes this local metadata
check without compilation. Fresh source release 6 and service release 32 retain
the same code and dependency scope; all native admission remains mandatory.
