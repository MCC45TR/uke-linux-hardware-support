# COPR source families

20 October 2026. Fedora Rawhide AArch64. This directory owns the Uke source
package map. COPR lists source packages in a flat list; it has no nested package
folders. Each family is one source RPM producing separately installable child
RPMs. Installing a metadata parent starts no process by itself.

| Source family | Child responsibilities |
|---|---|
| uke-core | uke-control and optional development diagnostics |
| uke-hw | retained boot/USB/Wi-Fi integration, separate installation classifier, firmware, Bluetooth factory-address oneshot, touchscreen and original Fastfetch configuration |
| uke-power | original UPower with reviewed corrections, battery observation/control gates, PPD backend, adaptive display |
| uke-sensors | SSC library, standard SensorProxy with SSC, bounded file service and opt-in front color |
| uke-additional | AT-SPI, GStreamer, libaccounts-glib, libwacom, C++ runtime and Mesa; original upstream versions retained |
| uke-extras | optional uke-plymonth theme, original material-decoration and recovery images delivered as data |
| uke-metas | separate strict console and original Fedora Plasma selections |
| uke-camera | optional original Fedora camera application selection; internal camera graph unqualified |
| uke-sound | original Fedora ALSA/PipeWire/WirePlumber selection; internal playback/capture unqualified |

The four kernel source names are `senemos-uke-linux-kernel-mainline`,
`senemos-uke-linux-kernel-mainline-singleboot`,
`senemos-uke-linux-kernel-mainline-kvm` and
`senemos-uke-linux-kernel-mainline-kvm-singleboot`. The resulting source list contains
13 families. The `material-decoration` binary keeps its original identity under
`uke-extras`; it no longer needs a separate source row. `uke-hw` also produces `uke-firmware`, an
explicitly requested proprietary data RPM with OEM notices. Its historical
file-level redistribution review remains incomplete; publication does not
establish additional grants. Private calibration is excluded.

Project application versions follow `MAJOR.MINOR.PATCH`: increment MAJOR for a
critical architectural or compatibility change, MINOR for a feature and PATCH
for a correction. Reset lower fields after a higher-field increment. A
packaging-only rebuild increments RPM Release. Upstream libraries, kernels and
third-party applications retain their actual upstream versions. Requested
application identities are `uke-control 1.0.19-1`,
`uke-adaptive-display 1.0.14-1` and `uke-plymonth 1.0.2-1`; the latter is the
requested package spelling, while the original Plymouth theme file paths stay
compatible. Do not add Epoch to outrank another repository.

The subsequent coordinated corrections use `uke-control 1.0.20-1` for F2FS
swap allocation and `uke-adaptive-display 1.0.15-1` for original KWin defaults.
The latter ships a generic monotone brightness curve, tablet-mode rotation and
automatic VRR policy only for DSI-1, without forced layout/resolution or factory
calibration. The configuration is installed with noreplace semantics; existing
user settings remain authoritative. These changes require their own native
build and device checks rather than replacing the earlier package identities.

The family composer retains complete exact-pinned source RPM inputs, original
specifications, archives, patches and licensing. It expands the reviewed RPM
recipes for the native target and combines their build/check phases in isolated
source directories. Binary RPMs are built from source, never copied from an
old binary package. Source input hashes and build IDs are in
`source-inputs.json`; component source changes require an explicit input refresh
and package revision review. Local Control, adaptive display, battery and meta
sources come from their current owning component recipes. The existing input
boot-22 keeps the frozen userdata image ABI intact.

Hardware source Release 2 retains boot22 and separately builds the corrected
`uke-installation-profile 1.0.31` from the coordinator's canonical C++ source.
The original classifier30 service unit is retained. Reconciliation runs only on
a real Uke outside a container/chroot, may repair only the exact legacy unknown
record, and preserves masks. It performs no block writes or slot changes. The
new A/B backend remains a separate future source freeze.

Firmware source inputs retain their exact upstream Version and Release.
Foreign DSP ELF data is held outside the buildroot while RPM runs native
post-install processing, then restored before the firmware hash checks. Native
programs retain normal post-processing. The final firmware RPM must match all
217 source file hashes and modes; a source check alone is insufficient.

The source family Release defaults to 1; set its `release` field when packaging
changes require a rebuild. Control's family version and Release must match its
owning spec; CI refuses a mismatch. Native child recipes keep their Release
once their source Version matches the declared native Version. Refresh exact
dependency pins deliberately: the Mesa LLVM build pin is 23.1.3-1.fc46 because
23.1.2-1.fc46 is absent from Rawhide AArch64. The original recipe is retained.

Renamed child RPMs carry versioned Provides (including ISA where appropriate)
and bounded Obsoletes for their previous names. New project names begin `uke-`;
standard executable paths, library SONAMEs, D-Bus names, units and pkg-config
interfaces remain upstream-compatible. Development packages retain exact
runtime dependencies. Meta selections do not force optional diagnostics,
recovery images, physical camera/audio controls or private calibration packages.
The Plasma selection stays independent of the Python-free console selection.

Service renaming preserves admitted system-unit state through RPM scriptlets;
a masked or disabled path is not newly enabled. SensorProxy's policy is restored
after the old package's removal scriptlet. Failed restoration retains its
private state for inspection. This is packaging logic, not physical lifecycle
qualification. The Bluetooth helper performs a device-specific factory address
handoff via the standard management API and exits; removing its package would
not remove a persistent extra Bluetooth daemon. UART, pinctrl, regulators and
protocols continue to belong to the kernel/DT and original BlueZ.

The Fastfetch fork is withdrawn. The normal Fedora `fastfetch` dependency is
retained. Control's signed native solver returns that one package explicitly
to `rawhide`, including a necessary downgrade; other project packages still
come from the selected Uke channel. Missing original replacements refuse the
transaction. Hardware identity fixes belong in the real kernel/DT and existing
system integration. Do not manufacture SMBIOS data or probe legacy PC memory
ranges on ARM to satisfy a display field. Full root all-module captures remain
unqualified until the original distribution program safely handles that case.

`uke-fastfetch-config` installs a noreplace system profile for original Fedora
Fastfetch, with dynamic CPU vendor/name, online cores and maximum-frequency
fields. It uses no command callbacks, BIOS/board/host detection or physical
memory module. Platform and advertised panel text are explicitly profile
metadata. Per-user configuration still overrides the system default. This is a
small data RPM with no helper or resident process, not a Fastfetch fork.

On Rawhide AArch64, build a source family with
`make -C packaging/families srpm FAMILY=uke-core`. The source-generation host
must match the target: RPM's `--target` does not retarget every build/host
triplet in an expanded recipe. The normal COPR source worker is AArch64.
Use `TARGET_ARCH=x86_64` only for isolated x86_64 host fixtures.
Run `make -C packaging/families check` and the original component/native solver
checks. Before changing COPR, validate every generated package name, dependency,
scriptlet, source hash, signature and rollback artifact. A COPR package deletion
also deletes its builds: never use it merely to hide a dashboard row. Preserve
referenced historical builds and test the new dependency closure before a
source-definition cutover. Stable promotion remains distinct from test builds.

Source-family automation watches each family's inputs, requires a higher RPM
revision, runs validation and suppresses duplicate active or published builds.
The COPR source definitions track the published branch with auto-rebuild enabled.
The direct GitHub integration accepts tag creation events; validated CI requests
changed families through the existing custom hook. This avoids two builds for
one branch push. Frozen third-party source inputs require an explicit hash and
revision refresh; auto-rebuild does not silently replace upstream sources.

Primary contracts: [RPM spec format](https://rpm.org/docs/6.0.x/manual/spec.html),
[DNF5 native transaction API](https://dnf5.readthedocs.io/en/latest/tutorial/transaction.html)
and [COPR source methods](https://docs.copr.fedorainfracloud.org/user_documentation.html#build-source-types).
