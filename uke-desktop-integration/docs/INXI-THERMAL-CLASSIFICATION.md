# SoC thermal classification

30 October 2026. On POCO Pad X1 with Fedora Rawhide AArch64 and Linux
7.3.0-54, Fedora `inxi-3.3.41-2.fc45` classifies the
`display-thermal-sensor` and `wifi-thermal-sensor` platform devices as
graphics and network hardware. Both bind `generic_adc_thermal` and expose
temperature measurements. Their device-tree names and bindings remain valid.

The patch makes `soc_type()` identify thermal node or driver tokens before
matching functional component names. The original source reproduces the
display classification error. The changed source passes thirteen actual
classifier cases, retaining display, GPU, Ethernet, Wi-Fi, audio, HDMI and
regulator classifications. A read-only device run omits the two thermal
consumers from graphics/network while preserving the real display devices
and USB Ethernet adapter.

The input executable has SHA-256
`47bfa3e3c5c922b96fa8cc50c566278d419c4cbcd501d42f30e973d4524c932d`;
RPM verification reports no installed-file changes. The Fedora Koji source
RPM has valid header/payload digests but carries no OpenPGP signature. It
is retained as packaging reference, not described as a signed source RPM.

Apply `patches/0002-inxi-soc-thermal-classification.patch` to the Fedora
inxi source and run `make test-inxi-soc-thermal INXI_SOURCE=/path/to/inxi`.
Keep the original package name, license, dependencies and Fedora update
policy. A package rebuild and installation are separate from the observed
execution of the patched source; no package takeover or DT rename is needed.

1 November 2026. `make inxi-srpm` rebuilds the exact retained Fedora source
with release 3 and the classifier patch. The original name, license, dependencies,
documentation and Fedora self-update restriction remain. The binary RPM runs the
thirteen classifier cases in `%check`. This candidate is restricted to the test
repository; it does not change Fedora's normal source or update policy.

COPR 11239935 produced signed source and noarch RPMs. On POCO Pad X1 with
Fedora Rawhide AArch64 and Linux 7.3.1-115, inxi 3.3.41-3.fc46 was installed
through a transaction that changed only inxi. Its dependencies match the prior
Fedora package; signature, payload and installed-file verification pass. Both
the graphical-user and root reports omit the thermal consumers while retaining
the real display and network devices. The thirteen classifier cases pass on
the actual binary payload. Future Fedora versions need the fix carried forward
until it is accepted upstream.
