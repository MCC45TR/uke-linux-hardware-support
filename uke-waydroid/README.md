# Waydroid integration

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

References: [Fedora Binder labels](https://github.com/fedora-selinux/selinux-policy/blob/rawhide/policy/modules/kernel/devices.te),
[Waydroid installation](https://docs.waydro.id/usage/install-on-desktops) and
[Binderfs lifecycle](https://docs.kernel.org/admin-guide/binderfs.html).
