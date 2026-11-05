# Waydroid integration

5 November 2026: official ARM64-only images can declare the
OMX HAL without shipping its service, blocking MediaProvider and shared
storage (upstream [2397](https://github.com/waydroid/waydroid/issues/2397) and
[2453](https://github.com/waydroid/waydroid/issues/2453)). A native pre-mount
helper reads the original ext4 images without writing or mounting them. On
exact Uke/ARM64-only MAINLINE configurations, it omits only that missing HAL
through an owned vendor overlay and selects the already provided Codec2 stack.
Original images, application data and administrator manifests are preserved.
Owned overlays retire if a later image supplies the service. This addresses
image metadata; it does not add hardware video codecs or establish game or
Google-account acceptance. The pre-mount overlay approach is also documented
in enij90/armada-tb321fu commitf430dd7; this implementation uses C++/Qt XML and
read-only libext2fs, without executing or shipping that donor's Python script.
COPR fixtures cover actual ext4 reads, retained HALs, idempotence, custom edits,
service admission, malformed XML and linked-directory refusal. Physical Uke
media/storage acceptance is recorded separately from those build checks.

Integration8, source9a8a9c9/COPR11262150, passes those native fixtures and signed
RPM review, then a targeted DNF update on POCO Pad X1/kernel12101-unstable.
The packaged pre-mount path restores FUSE shared storage and registered
software Codec2. The185-second guest recheck retains a stable MediaProvider
without matching OMX, ANR or storage-session errors. HTTPS204 and the complete
16384-pixel hardware Vulkan check pass. Original Android images and the private
Google registration value are preserved. This is container/session startup
qualification; a new system reboot, real media decoding/performance, games,
Google account completion and prolonged graphics stability remain separate.

4 November 2026. Fedora Waydroid1.6.3 creates `anbox-binder`, `anbox-hwbinder`
and `anbox-vndbinder` on a kernel with built-in Binder and an empty static device
list. On POCO Pad X1, kernel7.3.1-1500, the new nodes receive `binderfs_t` and
Enforcing SELinux denies the existing `waydroid_t` process read/write access.
The original Fedora Waydroid module grants those operations to
`binder_device_t`.

This package supplies three Binderfs path-label mappings to that existing type.
The daemon may also read the Binderfs directory to enumerate newly allocated
nodes. Three standard tmpfiles aliases are created before daemon startup,
including after reboot. The original Fedora Waydroid application,
SELinux module and Android images remain distribution/upstream components.
The COPR source worker checks the mappings against the complete original
Fedora policy with reload disabled; RPM installation registers the module.

Install `uke-waydroid-integration` through DNF alongside the original
`waydroid` package. The supplemental dependency selects it on normal Uke
installations with Waydroid and Core present. Existing sessions should be
stopped before recreating old dynamic Binder nodes and starting Waydroid again.
On the 7200 unstable kernel, a new boot retains the corrected device labels
but exposes missing `/dev/anbox-*` links and a Binderfs directory-read denial.
Revision2 addresses those observed startup prerequisites. Android boot and
graphics results still require separate device validation.

Revision4 enables the upstream `persist.waydroid.multi_windows` preference
for separate application windows on KDE Wayland. A small POSIX shell helper
merges that preference into the original configuration and generated base
properties during RPM installation and before daemon startup. It preserves
the chosen hardware renderer and unrelated configuration. Administrators may
change `/etc/waydroid/uke-defaults.conf`; later updates preserve that file.
The package does not start an Android window before the user launches an app.
Repeated startup, hardware rendering and actual window behavior still need
device qualification.
The original Binderfs mount is ordered before daemon preparation, so dynamic
device allocation does not race the boot-time mount.

Revision5 also supplies `mesa.fd.dev.features=enable_tp_ubwc_flag_hint=0`
and `ro.hardware.vulkan=freedreno` only on `xiaomi,uke`. The installed Android
image has Mesa26.0.8 and the native Freedreno Vulkan library, while the original
Waydroid GPU map does not recognize the separated kernel driver name `adreno`.
The hint must agree with the installed host Mesa correction. This setting
retains UBWC storage and hardware acceleration; it needs a separate physical
corruption/restart check. See [Mesa Android properties](https://docs.mesa3d.org/android.html#android-system-properties).

On4 November, revision5 on kernel7200 reaches Android13 boot completion and
reports hardware Freedreno GLES3.2. Calculator and Settings have separate KWin
window identities and normal taskbar admission. Visual stability still needs
the physical observer's result. Kernel7200 lacks Android's multiple-table
routing and in-container legacy filter support, blocking external networking;
the separate unstable kernel candidate supplies those requirements.

Revision6 pulls the small `uke-waydroid-diagnostics` package through DNF. Its
Android-side C++ tool reports the actual Vulkan device/version and compares a
4KiB GPU buffer fill. The old Android13 `vkjson` serializer has no device-format
case above Vulkan1.3, so an empty device object alone is not a driver failure.
The diagnostic is installed in the original Waydroid overlay and runs only
when explicitly invoked with `waydroid shell -- /system/bin/uke-vulkan-check`.
Its NDK sysroot and native compiler are confined to COPR. No SDK/compiler is
installed on the tablet, and this bounded check does not establish conformance.

Diagnostics revision3 also draws sixteen alternating red/green triangles into
a32×32 offscreen Vulkan attachment. Independently calculated pixel coverage
checks both the triangle and cleared background after every GPU submission.
Shader compilation/validation runs only in COPR. This exercises vertex/fragment
shaders, graphics-pipeline creation, render-pass synchronization and image
readback; it does not measure game performance or test Android window/swapchain
presentation, textures, protected content or full API conformance.

On4 November, the physical observer confirms separate Calculator/Settings
windows without corruption in the bounded integration6/kernel7200 trial.
After a separate boot into8101, the hardware buffer check again passes with
zero mismatches and GLES remains Freedreno3.2. Policy routing and legacy
filtering remain unavailable on8101; Play Store access requires the capable
network kernel and a separate application test. Android13 and the available
community ARM64 Android16 image both report aFebruary2026 security baseline;
an image's build date alone is not its security patch date.

On the Uke unstable channel, revision7 requires the selected singleboot profile
to provide `uke-kernel-default(waydroid-netd) >= 1`. This lets a normal DNF update
pull the matching network-capable kernel cohort and prevents replacement of
that default profile by a candidate missing the capability. Installed/default
profile selection and the actually running kernel remain separate: existing
administrator boot-activation policy is respected.

The4 November11003 unstable boot retains integration7 and diagnostics3 through
normal DNF delivery. Android boots, IPv4/IPv6 policy rules and legacy filter
tables work, and guest HTTPS connectivity returns204 with an Android VALIDATED
network. The hardware probe passes all16384 pixels in16 shader-rendered frames
with zero mismatches. Google-account registration/sign-in, prolonged physical
display stability and real game performance remain separate acceptance steps.
The existing automatic staging and user-selected rollback policy are preserved;
this result does not promote the normal/stable kernel family.

References: [Fedora Binder labels](https://github.com/fedora-selinux/selinux-policy/blob/rawhide/policy/modules/kernel/devices.te),
[Waydroid installation](https://docs.waydro.id/usage/install-on-desktops) and
[Binderfs lifecycle](https://docs.kernel.org/admin-guide/binderfs.html).
