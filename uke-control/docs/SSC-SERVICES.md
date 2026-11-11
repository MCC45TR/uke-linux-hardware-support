# SSC service ownership in Control

Source candidate, 12 November 2026. This consolidation moves the current
SSC file-service/cache owner into the existing Control package. It does not
claim an original Fedora provider transition or additional hardware support.

The exact installed SSC32 inputs, six patches, cache, units, udev rule and
consumer ordering are preserved. Runtime paths, private RAM registry, shared
reader lock, atomic refresh, read-only persist and native-label copy remain.
The old RPM name and OEM-defaults capability are provided virtually by Control.
DNF obsoletes only the old SSC package; Core and Adaptive dependencies remain
satisfied. Four unit states are saved before the ownership transaction and
restored afterward in dependency order without enabling a disabled service.
New explicit masks take precedence. Invalid or stale snapshots are refused.

The current protocol helper remains a temporary compatibility provider because
the original Fedora library has not yet passed complete live namespace,
registry, SELinux, crash/suspend and sensor parity. Its exact remaining delta
from linux-msm/hexagonrpc 510c87e5 is: sequence/u64 types, OEM/ODM path aliases,
mapped symlink rejection, short shell-image rejection and listener readiness.
No additional protocol delta is introduced here. Removing this fallback before
measured parity would reduce current functionality.

The return path is the unmodified Fedora fastrpc library with Control's small
one-session/notification integration and standard private Linux namespaces.
That candidate is packaged as internal helpers but not selected by these units.
The original fastrpc library package contains no autostart units/rules;
fastrpc-services is not required. Kernel/DT retains transport, topology,
resource ownership and lifecycle. Desktop/session policy remains userspace.
Remove the compatibility executable after live parity and supported rollback.

Source worker inputs are the exact Control4 SRPM, SSC32 integration SRPM and
native102 qualification SRPM. All parent Control sources and original SSC
inputs are hash checked. The complete original-library readiness, file API,
session and ABI fixtures run alongside existing Control checks in COPR.
Legacy registry/shell/readiness/cache tests remain, as do malformed/masked/
disabled migration checks. No native compilation runs on the tablet.

Before retirement, verify signed outputs, payload/ELF/config/ownership parity,
fresh/reinstall/archived rollback solves, normal DNF, private registry generation,
read-only persist, unit states, ADSP/WPSS and CDSP ownership, then refreshed
unstable metadata absence. Source/build success is not physical acceptance.
Keep gain223 and all cumulative kernel/recovery preferences.
