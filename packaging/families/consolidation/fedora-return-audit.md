# Original Fedora package comparison

1 November 2026. Fedora Rawhide 46 AArch64 RPMs were inspected in a Fedora
source-validation container. No library, helper or package transaction ran on
Uke. The [input manifest](fedora-return-inputs.json) identifies 13 signed
original/fork pairs, immutable download locations and complete SHA256 hashes.
It does not admit replacements.

The later [desktop return result](fedora-return-results.md) records the three
admitted source families and their separate migration checks. The GNU and
libaccounts limits below remain in force.

The static comparison passed for 13 shared libraries. The original Fedora
libraries retain the reviewed exported-symbol versions and SONAMEs. All 219
header, pkg-config, GIR and typelib files and 836 libwacom database files
match. Both signing identities were checked in an isolated RPM key database.
A missing-symbol negative control was rejected. The result is recorded by
[source CI 41277779395](https://github.com/MCC45TR/uke-linux-hardware-support/actions/runs/41277779395)
at `a4f321c3b48b9dd740ec33beb553b6c265e67235`.

The original `gst-hotdoc-plugins-scanner` is a native executable.
`gst-plugins-doc-cache-generator`, `libwacom-update-db` and
`libwacom-show-stylus` are distribution Python helpers. The original at-spi
GI override and GNU GDB pretty-printers also contain Python. These files must
remain within the approved original-Fedora desktop scope; they do not justify
Python in project services, startup, recovery, initramfs or installers.
The GNU runtime remains conditional on its console-profile boundary and all
`senemos-native-runtime(libstdc++)` consumers, including boot and power packages.

Standard Fedora packages do not advertise Uke-prefixed aliases. Source
withdrawal alone cannot migrate installed clients or optional development
packages. A compatible manager, original-candidate admission, forward/reverse
dependency plans and preserved rollback must be verified before changing the
four producer routes. The draft manager also needs to retain qualified Control
2.7 features alongside the independently signed unstable channel and current
updater transaction policy. None of those migrations is claimed here.

`libaccounts-glib` remains excluded from a policy-only return: the reviewed
Fedora source lacks the upstream destructor correction documented in the
[upstream difference report](upstream-delta-report.md). No kernel, DT, firmware,
private calibration or device support result changes with this comparison.
