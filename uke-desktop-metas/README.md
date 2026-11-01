# Uke desktop selections

29 October 2026: `uke-cutefish-meta` selects the real `cutefish-desktop` RPM from
the separate `mcc45tr/cutefish` upstream-packaging project. It enables no display
manager, creates no account and requires no KDE meta package. This source-only
selection is removable. Graphical login, image composition and tablet tests
remain pending; package installation cannot establish those results.

25 October 2026: release `1.0.0-19` recommends Chromium rather than requiring
it. Default DNF weak-dependency handling installs it with KDE, while removing
the browser no longer requires removal of the KDE meta package. Browser data
and original Fedora packages are not modified by this metadata change.
25 October 2026: source revision 23 requests explicit JSON from Control 2.0 and
uses the canonical CDC/USB-role commands. The dependency/conflict pair requires
the matching CLI/client cohort while preserving separate widget removal. Host
command/schema and QML checks are separate from native packaging and tablet use.

Chromium is recommended by default and can be removed while retaining the KDE
meta package. This preserves the packaging correction in release 19.

25 October 2026: source release `1.0.0-22` requires Control 1.13.0. Torch on/off
and brightness use fixed unprivileged client commands. The 1–100 slider shows
the requested percentage separately from the two physical lit steps. The
separate widget package remains a removable KDE recommendation and is excluded
from the GNOME selection. QML loading and command/schema fixtures pass on the host;
signed package and tablet acceptance remain separate.

25 October 2026: source release `1.0.0-21` requires Control 1.12.0. Regular
RGB color/brightness controls are removed. One **Test colors** button requests
the kernel's ten-second color transition without sudo or pkexec. It remains
unavailable without the matched kernel device and confined service. Torch
brightness is retained. The actual applet component and cache check pass on
the host; no new RPM or tablet result is claimed for this source revision.

25 October 2026: source release `1.0.0-20` requires Control 1.11.0. The widget
adds a 0–100 torch brightness dialog and RGB components/brightness with a color
preview. Apply sends one validated backend request. Percentages stay within
the retained limits; hardware torch steps are 0/50/100. No package or tablet
result is claimed for this source revision.

25 October 2026: release `1.0.0-17` requires Control 1.10.2 with corrected
original LED-class provider discovery and the bounded, suspend-aware supervisor.

25 October 2026: release `1.0.0-16` requires Control 1.10.1 so the flashlight
supervisor includes sleep ordering and a suspend-aware deadline.

25 October 2026: release `1.0.0-15` adds a bounded flashlight switch and low-duty
red/green/blue/off controls as clients of Control 1.10.0. Missing exact kernel
providers keep the rows unavailable. The widget performs no hardware writes.

24 October 2026: release `1.0.0-14` gives QML sources a distinct reproducible
timestamp for each monotonic RPM revision. This invalidates Qt's older disk
cache after an update. The package check covers the cached invalid-to-corrected
transition as well as the actual applet component.

24 October 2026: release `1.0.0-13` fixes widget loading on the original
Fedora Qt 6.11 stack by sizing the USB confirmation dialog instead of its
label's read-only implicit width. The package check and source CI compile
the actual applet QML component with original Plasma imports. This check
does not create the widget or invoke device commands; tablet presentation
and physical controls remain separate validation.

24 October 2026: `uke-kde-plasma-meta` recommends the separate
`uke-plasma-controls` RPM. The Uke Tablet Controls entry appears in the original
Plasma system tray. Every device request goes through `uke-control`; the applet
contains no hardware backend. The recommendation installs by default with KDE
and can be removed with `sudo dnf remove uke-plasma-controls` while retaining
the KDE meta package. Default DNF weak-dependency policy respects prior removal.
No GNOME dependency or integration is installed. Tablet UI acceptance remains
separate from package and host checks.

`uke-desktop-metas` records the console-first desktop policy. The optional
`uke-kde-plasma-meta` selects original Fedora Rawhide AArch64 Plasma applications.
It includes Discover's PackageKit backend, Dolphin plugins and administrative
access, KWrite, Chromium, display settings and the common desktop utilities.
Neither package contains a desktop application fork or a hardware service.

The Plasma selection includes original Fedora `plasma-login-manager` and
`kcm-plasmalogin`. It installs no login preset or display-manager activation scriptlet. On an
existing graphical installation, select the standard service explicitly:

```sh
sudo systemctl enable --force plasmalogin.service
```

Remove any previous display manager's explicit startup links before rebooting;
only one manager may run. Preserve the previous package for rollback during
validation. Sensor startup does not depend on a specific login manager.

24 October 2026: the Plasma selection keeps original Fedora `kde-connect` and
its session autostart. The packaged `uke-kdeconnect` firewalld policy permits
the distribution's KDE Connect service from private IPv4 ranges and IPv6
link-local/unique-local addresses, including networks in the `public` zone.
Other traffic continues through the existing firewall policy. No application
fork, extra daemon, automatic pairing or user autostart override is installed.
Installing, upgrading or removing the selection uses Fedora's firewall reload
macro; a reload retains connections and replaces runtime-only firewall edits
with permanent configuration. Globally routed and VPN-specific address ranges
need an administrator's explicit rule if they are used for KDE Connect.

To disable the packaged network permission, preserve an administrator override:

```sh
sudo firewall-cmd --permanent --policy=uke-kdeconnect --add-disable
sudo firewall-cmd --reload
```

The override survives package updates. Existing KDE Connect pairing records and
per-user autostart choices remain authoritative. Source/package validation and
actual discovery on the tablet are recorded separately.
1 November 2026. Unmodified original Fedora RPMs retain their provided helpers
and dependencies on the Fedora root, including GNU C++ helpers in console
profiles. Project components use C++ or shell. Recovery, initramfs and installers
remain Python-free. The Plasma selection remains independent of the Core
hardware selection.

The application list was compared with the Nabu KDE manifest and image profile at
[`03a5bdf09ce206410e7f7f1f97871110e66ffa82`](https://github.com/MCC45TR/Nabu-Fedora-Rawhide-Builder/tree/03a5bdf09ce206410e7f7f1f97871110e66ffa82).
Nabu hardware packages and first-run setup are excluded. Uke display,
touch, graphics and sensors require their own drivers and physical tests.

With Fedora `firewalld-filesystem` and `libxml2` installed on the build host,
run `make validate`, `make test-native` and `make srpm` for source and policy
checks. COPR's root `.copr/Makefile` builds both RPMs from this source family.
The optional Plasma RPM owns `plasma.json`, which records its exact application
selection for image composition. Install it explicitly on a Fedora graphical
root; installing the console metadata alone does not select Plasma.

As of 18 October 2026, the revised profile is a package candidate. RPM build, dependency
resolution, graphical application behavior and tablet hardware acceptance are
separate checks. Detailed test records remain in the private project journal.
