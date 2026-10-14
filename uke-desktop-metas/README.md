# Uke desktop selections

`uke-desktop-metas` records the console-first desktop policy. The optional
`kde-plasma-uke-meta` selects original Fedora Rawhide AArch64 Plasma applications.
It includes Discover's PackageKit backend, Dolphin plugins and administrative
access, KWrite, Chromium, display settings and the common desktop utilities.
Neither package contains a desktop application fork or a hardware service.

The Plasma selection retains the installed login manager. It installs no login
preset and does not start a graphical session. The core console selection keeps
its strict no-Python policy. Since 14 October 2026, unmodified original Fedora
desktop RPMs may retain their Fedora-provided Python dependencies on the Fedora
root. This exception does not cover project-owned code, recovery, initramfs or
installers. The Plasma meta is independent of the strict console meta so these
policies do not conflict during dependency resolution.

The application list was compared with the Nabu KDE manifest and image profile at
[`03a5bdf09ce206410e7f7f1f97871110e66ffa82`](https://github.com/MCC45TR/Nabu-Fedora-Rawhide-Builder/tree/03a5bdf09ce206410e7f7f1f97871110e66ffa82).
Nabu hardware packages and its login-manager migration are excluded. Uke display,
touch, graphics and sensors require their own drivers and physical tests.

Run `make validate`, `make test-native` and `make srpm` for source and policy
checks. COPR's root `.copr/Makefile` builds both RPMs from this source family.
The optional Plasma RPM owns `plasma.json`, which records its exact application
selection for image composition. Install it explicitly on a Fedora graphical
root; installing the console metadata alone does not select Plasma.

As of 14 October 2026, the profile is a package candidate. RPM build, dependency
resolution, graphical application behavior and tablet hardware acceptance are
separate checks. Detailed test records remain in the private project journal.
