# Battery packaging candidate

The Fedora AArch64 `uke-battery-observer.spec` produces the original read-only
observer package and a separate root `uke-charge-control` subpackage. It contains
no firmware, UPower source, installed tests, Python payload or factory data.
All service presets are disabled. D-Bus may activate only the unprivileged
read-only observer; it cannot activate the control helper. The control package
requires reviewed kernel/profile admission and is unqualified by default.

The recipe, native dependency closure, sysusers/preset behavior and upgrade/
removal paths for this candidate have not been built or tested. Other intended
runtime packages remain unavailable. Review `docs/BATTERY.md` and
`docs/CHARGE-CONTROL.md` before joint validation. The control/profile digest uses
the distribution's OpenSSL libcrypto; `openssl-devel` is a host build dependency,
and generated ELF dependencies must retain the target libcrypto requirement.

`upower/` contains a separate optional source RPM recipe for the reported Fedora
1.91.5 daemon. It retains the upstream ABI/original clients and excludes installed
Python tests. Its host-only upstream build dependencies, exact sources and joint
test boundary are documented in `upower/README.md`. No package was built here.
