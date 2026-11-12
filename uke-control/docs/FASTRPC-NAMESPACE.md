# SSC filesystem namespace qualification

Software fixture,12 November2026;POCO Pad X1 with kernel7.3.3-24-unstable and
original Fedora systemd262.2. Only public synthetic data and original Fedora
shell/coreutils were used. No DSP node,factory persist,calibration or production
sensor service was opened or changed. The transient unit and temporary tree
were removed after the run.

The existing HexagonRPC virtual mappings can be expressed by a private
RootDirectory and standard bind mounts:

| DSP path | Current cache source | Access |
| --- | --- | --- |
| /persist/sensors/registry | state/sensors/persist | read/write parent |
| /mnt/vendor/persist | private /persist alias | same bind mount |
| /vendor/etc/sensors/config | state/sensors/config | read-only |
| /odm/etc/sensors/config | same config | read-only alias |
| /vendor/etc/sensors/sns_reg_config | state/sensors/sns_reg.conf | read-only |
| /system/vendor | private /vendor alias | same bindings |
| /usr/lib/qcom/adsp | state/dsp/adsp | read-only |
| /sys/devices/soc0 | state/socinfo | read-only |

The target fixture demonstrates a shared registry mutation through both aliases,
equal config aliases,read-only config/firmware/SoC write refusals and unchanged
source hashes. DSP devices,host kernel-update configuration and remoteproc sysfs
are absent in that fixture. It does not exercise real FastRPC file requests,
all SoC attributes,live calibration,service restart or suspend.

The actual SSC32 build_persist_dir maps the complete registry parent when
state/sensors/persist exists. It contains registry/,sns_reg_version and the
temporary fstempfile used before a rename. The earlier leaf-only fixture did
not model this layout. The corrected fixture verifies parent temporary-file
creation and rename into the registry across both aliases. A fixed private
alias uses one bind mount, preserving atomic rename across the two names.

Root construction must include the AArch64 interpreter at
/lib/ld-linux-aarch64.so.1. Bind the original /usr/bin,/usr/lib64 and the
interpreter read-only;do not expose the entire host /usr just to start a daemon.

In systemd262,PrivateNetwork=yes inserts a private sysfs mount with priority
over TemporaryFileSystem=/sys. The relevant ordering and insertion are in
[namespace.c](https://github.com/systemd/systemd/blob/v262/src/core/namespace.c).
MountAPIVFS=no alone does not cancel that separate network-namespace behavior.
The fixture uses a private mount namespace,an empty read-only /sys with the
selected SoC subtree,and an address-family allow-list. It omits PrivateNetwork,
matching the existing SSC units. Live FastRPC address-family requirements still
need qualification. No systemd fork or global policy change is involved.

The extended target fixture also mounts an empty, read-only /proc. It refuses
host and /proc/1/root symlink escapes from the writable synthetic registry,
and a symlink cannot bypass a read-only config bind. Input hashes remain equal
and the transient unit/tree are removed. These are public software vectors;
no factory data, production service or DSP is touched. Production confinement
must keep this boundary and expose only required executables and devices.

Source and native readiness are separate:
[COPR11310322](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11310322/)
qualified the original Fedora listener/stubs and Control notification adapter
with synthetic RPCs. Combining that result with this filesystem fixture does
not prove a complete provider switch. Native100 qualifies 87 original library
file API checks and native102 qualifies the single-session worker and source
ABI. Live library/config paths,boot/autostart ownership,readiness,
private-RAM registry refresh,SELinux,crash/suspend and signed DNF rollback
still require qualification for the original provider.
Retain the current provider until those gates pass.
