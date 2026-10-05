# Uke core package selection

`uke-core-meta` is a real AArch64 RPM/SRPM with a minimal console dependency
selection and a machine-readable Uke readiness profile. It requires the reviewed
Senemos Uke kernel family, NetworkManager, OpenSSH and basic native/shell tools.
It does not install unqualified firmware or Nabu hardware services. The current
candidate does not establish tablet boot, a working console or hardware support.

`make validate` checks source policy; `make srpm` creates a complete source RPM.
COPR's isolated source factory uses `.copr/Makefile`. Pushes to `main` trigger
native Rawhide AArch64 builds after the source webhook is enabled. Target closure
and actual install/upgrade/removal results are recorded separately in `reports/`.
No repository, kernel boot selection, Android partition or service preset is
modified by this package's own payload; it has no scriptlets.

The [development COPR](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-test/)
and [package catalog](https://github.com/MCC45TR/uke-linux/blob/main/docs/PACKAGE-HUB.md)
track each independently admitted component. Source archives stay in `referances/`.
Nabu's core specification at commit `98188b595b42ba975f5bc238e596f330d3994ed8`
was read as a role reference; its boot, calibration, panel, SSC and firmware
payloads were not copied into this Uke package.

Release 3 requires `senemos-native-runtime(libstdc++)`. The base Fedora C++
library contained Python GDB helpers despite declaring no interpreter runtime;
the complete-root gate rejected that environment. The native replacement is
source-built with ABI/smoke checks. Interpreter conflicts remain necessary,
and complete installed-root inspection is an independent required gate.
