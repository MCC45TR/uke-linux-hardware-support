# UKE-NATIVE-PACKAGES-20261111: distinguish ownership coverage from retirement

Date: 11 November 2026. Environment: POCO Pad X1, Fedora Rawhide AArch64,
kernel `7.3.3-22-unstable`, installed Control/TUI `2.17.0-1`.

The current public unstable COPR list has 33 producers; the installed RPM
database has 36 distinct `uke-*` names. Neither number measures hardware
ownership or functional parity. ddcutil and Qt Virtual Keyboard retain original
package names despite being project rebuilds; an absent producer does not
change the installed binary. Split packages also have different runtime and
source identities: the battery producer remains required for observer/offline
roles after charge-helper ownership moved to Control.

The native-migration inventory and read-only audit now correlate these source
and binary populations. The real snapshot has no unmapped producer or selected
installed source. Tests refuse partial, duplicate and wrong-project data,
surface unknown unprefixed rebuilds and preserve split-source remaining work.
Source/vendor labels are not signature proof. All retirement and functional
acceptance fields remain false.

The project source entry points now share a retired-source check. It covers
standalone generation, umbrella/archive inputs and direct native-runtime
preparation before downloads/builds. The tests use build/download stubs to
verify that legacy paths cannot resurrect the qualified PipeWire, libwacom,
GStreamer, accessibility or GNU runtime forks. Necessary current providers
are not retired and original Fedora dependencies are not filtered.

The first test run found a missing parenthesis in the jq inventory filter;
it was corrected before live admission. `make` is not installed on the tablet,
so the added Make target was not run locally; its exact Bash suite passed
directly. Shell syntax and whitespace checks passed. No native compilation,
COPR job, package transaction, kernel staging, firmware SET or hardware test
was performed in this source change.

Initial full source CI failed in the umbrella fixture's worker-identity check,
before any production/download stub ran. The container's OS release spelling
did not satisfy the real native-worker gate. The fixture now models that
worker identity alongside its existing RPM/tool stubs; production OS admission
is unchanged. Runs 43044835314, 43044987921 and diagnostic 43045420035 retain
the failed evidence. This correction does not weaken the retired-source check.

Original UPower's unknown-health and late-capacity behavior and SensorProxy's
magnetic/lifecycle parity still prevent their retirement. Graphics/video
userspace compatibility and Qt key sound also remain open. Kernel/DT must not
fake data or absorb desktop/compiler policy to bypass these gates. Next
acceptance is signed original-consumer equivalence, supported DNF transitions
and recoverable physical tests. No hardware risk or broader goal is closed.
