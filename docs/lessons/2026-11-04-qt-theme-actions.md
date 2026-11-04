# UKE-QT-THEMES-20261104: keep semantic icons and request routing native

Date: 4 November 2026.
Environment: Fedora 46 Rawhide aarch64 on POCO Pad X1 (xiaomi,uke),
running kernel 7.3.1-1500. Native compilation is restricted to COPR.

## Source finding and decision

The shipped Qt interface used one generic icon for reads and another for changes
across the full action catalog. Its daily Updates page omitted channel selection
and channel preview even though both existed in the installed SDK. Read forms
also described their options as draft preferences.

The frontend now supplies semantic theme names for catalog actions, navigation,
read states and review controls. The user-selected theme remains effective.
KDE icon-change notifications refresh the bindings; missing names use installed
theme inheritance and fallback. Appearance metadata checks actual pixmap
rendering rather than treating a non-null icon handle as sufficient.
See the official [QIcon lookup rules](https://doc.qt.io/qt-6/qicon.html)
and [KIconLoader change notification](https://api.kde.org/kiconloader.html).
No distribution artwork is copied and no KDE package is forked.

Daily Updates now exposes channel status, selection, preview, update preview
and installation. Guide actions are available in the page body and toolbar.
Read options, requested preferences, errors and successful terminal handoff
messages have separate wording or severity. A provider reporting
`available:false` is labeled unavailable even when its reporting command exits
successfully. No measurement or physical success is invented.

## Validation boundary

Native tests exercise every catalog guide and typed request in fixture mode,
theme resolution, accessible action labels and actual QML read/refresh/stop
controls. An optional tests RPM allows these compiled checks to run after DNF
installation without compilation or copied executables on the tablet.
Fixture review remains blocked even with exact consent and confirmation tokens.
Build, package installation and physical provider results must be recorded
separately below after they are observed.
