# Retire the installed Qt test package

11 November 2026. The Qt interface and Waydroid installer remain project
applications using original Fedora Qt/KDE libraries. This packaging update
retains their exact1.6.0-6 source archive, `BUILD_TESTING=ON` and the complete
native CTest run. It removes test payload only after successful build checks,
and stops publishing the separate test/debug RPMs. The Qt runtime obsoletes
the old test package during a normal DNF upgrade.

The source worker consumes the exact archived6 SRPM, verifies its source/spec
hashes and proves a whole-spec inverse. The two runtime output names stay
unchanged. Native build, signed runtime payload/ABI comparison, DNF upgrade,
preferences and repository cleanup require separate evidence.

Keep signed6 runtime/installer recovery and its complete source archive before
deleting superseded COPR builds. Retiring a test subpackage must not delete the
current GUI producer or suppress its native build tests. The main Control/TUI,
kernel, firmware, sensor and charging providers are outside this change.
