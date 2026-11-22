# Uke desktop package sources

This repository owns decoration, theme and native runtime sources for Fedora Rawhide AArch64:

- `material-decoration`: optional upstream C++20/Qt6 KWin decoration and KCM.
  Source is pinned to the published upstream release in
  `manifests/material-decoration.json`, with archive SHA-256 verification both
  before SRPM generation and during RPM preparation. Its GPL/LGPL source license
  is retained. The small KPlugin metadata correction is attributed to the Nabu
  reference specification at `152980fa9a4fd2d71a2825efe10411c5a8a74da7`.
- `uke-plymonth`: removable Xiaomi/Fedora boot, shutdown and update visuals,
  adapted from the MIT-licensed Nabu theme. Original Fedora Plymouth services
  start the graphical theme after mounting root, with pixel scaling fixed to one.
  The original label plugin and fonts are required for text and console rendering;
  the package build checks actual text dimensions as well as script syntax.
  No initramfs, boot image or slot is modified. See the
  [source and validation record](src/plymouth-uke/README.md).

`make validate` checks the source contracts. `make srpm PACKAGE=NAME` creates
that family's complete SRPM. The repository-root `.copr/Makefile`, invoked from each package subdirectory, exports only its
source RPM; source work stays outside COPR's unprivileged collection directory.
COPR builds binaries without network access. Main-branch push hooks rebuild both
families. The Material stable tracker fetches published upstream releases, pins
commit/archive identities and requests a new native build; failed compilation
does not establish package acceptance. New findings and native transaction
results have distinct `docs/lessons/` and `reports/` records.

Fedora's shared `powerdevil` and `plymouth` engines are dependencies. No measured
Uke issue currently justifies replacing them with Nabu-patched generic RPMs.
The source catalog retains those reference families for later Uke-specific work.
Project payloads use C++ or shell. Original Fedora packages retain their
distribution-provided helpers and dependencies on the Fedora root.
Neither decoration compilation nor theme installation proves Uke graphics or boot.

[Uke package hub](https://github.com/MCC45TR/uke-linux-docs/blob/main/docs/PACKAGE-HUB.md)
· [Development COPR](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-test/)

1 November 2026. The original Fedora accessibility, GStreamer, libwacom and
GNU C++ packages replace four policy-only source variants. Their Uke source
offers remain archived. The active [native runtime catalog](docs/NATIVE-RUNTIME.md)
retains Mesa, libaccounts-glib, UPower, IIO-Sensor-Proxy and AccountsService for
their functional corrections. Hardware acceptance remains separate.

Recovery, initramfs and installers remain Python-free. Project components
do not introduce Python hardware or startup services.

A sixth source candidate, [Mesa](packaging/mesa/README.md), corrects an observed
AArch64 llvmpipe SIGILL by passing Linux-reported CPU features to Gallivm.
It uses the complete pinned Fedora recipe with one source conditional changed;
KDE applications are untouched. Its native COPR regression, signed package and
physical Wayland gates remain separate from the five existing automatic builds.

The [package testing procedure](docs/PACKAGE-TESTING.md) requires both the
selected RPM closure and the complete installed root. The original 603-input
transaction passed its input audit and lifecycle fixtures, but its base image
contained optional Python GDB helpers in libstdc++; complete-root acceptance
was rejected. The GNU C++ source passed all 6,100 original versioned exports,
native AArch64 smoke and signed payload checks in build 11077014. A separate
52-input console selection passed offline installation, actual earlier metadata
upgrade, complete inherited-root audits and removal. KDE variants remain
withdrawn and graphical admission blocked. The builder records exact identities,
accepted gates and excluded failed trials separately.

Project policy prohibits cloning, forking or rebuilding KDE desktop applications
as Uke variants. Plasma and Dolphin source targets are withdrawn, their automatic
COPR jobs are disabled, and the source allowlist rejects them before download.
Use original distribution KDE applications. A development exception recorded
on 12 October covers
Python only in unavoidable original Fedora KDE dependencies; the image builder
audits those exact original files. Project runtime and early boot retain the
Python prohibition. Earlier source, helper
and package test records are historical evidence, not admitted runtime recipes.
