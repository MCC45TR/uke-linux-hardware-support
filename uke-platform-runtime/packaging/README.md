# Battery packaging candidate

The Fedora AArch64 `uke-battery-observer.spec` packages the original C++ observer
and read-only service. It does not package a second charger or any firmware,
UPower source, tests, Python payload or factory data. Its service preset is
disabled. D-Bus may activate the unprivileged read-only observer when requested.

The recipe, native dependency closure, sysusers/preset behavior and upgrade/
removal paths have not been built or tested. Other intended runtime packages
remain unavailable. Review `docs/BATTERY.md` before joint validation.

`upower/` contains a separate optional source RPM recipe for the reported Fedora
1.91.5 daemon. It retains the upstream ABI/original clients and excludes installed
Python tests. Its host-only upstream build dependencies, exact sources and joint
test boundary are documented in `upower/README.md`. No package was built here.
