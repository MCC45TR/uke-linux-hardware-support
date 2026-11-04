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

COPR build 11255240 compiled successfully and passed CLI, appearance and retained
widget checks. The QML test inspected an incoming page while Kirigami still
disabled it for animation. The check now reacquires the visible control until
the transition completes, avoiding a stale pointer to a departing page.
This failed package was not installed. Semantic charging and flashlight keys
also gained the installed Breeze aliases `battery-100-charging` and
`flashlight-on`; those two abstract names are not direct Breeze filenames.

Build 11255273 passed the full typed request loop and theme tests, then exposed
empty accessible names on FormButtonDelegate despite visible text. Explicit
names now cover catalog and guide actions, review details and raw output.
Plain-text information delegates also expose their label and description.
This is a frontend accessibility correction; release 5 was not installed.

## COPR and tablet results

COPR build 11255293 produced `1.0.0-6.fc46.aarch64` after all four CTest suites
passed. DNF installed the GUI and optional tests RPM on POCO Pad X1, replacing
GUI release 3 without other package or kernel changes. The normal Wayland
session reports Breeze, `org.kde.desktop`, the selected `breeze-dark` icon theme,
and 48 renderable icons. Native charging/flashlight names resolve to the expected
Breeze aliases. The dark narrow-window Updates page, channel form, read result
and review sheet were visually inspected from fixture screenshots.

The installed Kirigami suite passed all eight tests on Wayland, including all
126 guides, fixture request routing, review refusal and four real read-only
queries: battery, charging, selected channel and microphone status. Each query
reported exit zero; this does not establish physical acceptance, EEPROM
calibration, focus, charging cutoff or any destructive operation.

The installed appearance suite exposed a test-isolation defect: the session's
`XDG_CONFIG_DIRS` included KDE defaults selecting Breeze Dark even though the
probe isolated `XDG_CONFIG_HOME`. Release 7 isolates both configuration search
paths in its temporary subprocess. The real application continues to inherit
the user's configuration; only the test probe is isolated. The other ten
appearance checks and the retained widget checks passed before this correction.

An isolated Breeze Light palette with Adwaita icons passed all guide navigation
checks. Visual inspection nevertheless found the drawer handle's missing-icon
placeholder: Kirigami defaults to `application-menu-symbolic`, while Adwaita
uses `open-menu-symbolic`. The application now sets the drawer's supported
`handleClosedIcon`/`handleOpenIcon` properties through the same resolver, preferring
the available menu/close symbolic aliases. Native drawer open/close and both
bindings gain runtime checks. This uses the original distribution control;
no Kirigami source is modified. See the upstream
[OverlayDrawer properties](https://api.kde.org/qml-org-kde-kirigami-overlaydrawer.html).

## Final delivery

COPR build [11255477](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-unstable/build/11255477/)
built release `1.0.0-8.fc46.aarch64` from source commit
`14096f160bbefb543b0a82c4aa22fa35642d044d`; all four CTest suites passed.
The package's SCM producer is pinned to that same commit. Release 7's isolated
appearance correction also passed COPR checks in build 11255430; it was not
installed on the tablet.

DNF upgraded only the GUI and its optional tests package from release 6 to 8.
Installed RPM verification passed. The installed checks completed on the real
Wayland session: 15 retained widget checks, 11 appearance checks and eight
Kirigami checks passed, plus CLI policy. This includes all 126 guide/request
routes in simulation and the four explicitly enabled real read-only queries.
An additional isolated light/Adwaita QML walkthrough passed; its menu icon and
drawer open/close were visually verified alongside the normal dark/Breeze UI.
All 50 registered icon keys render, including both native navigation handles.

Both root previews, `update --check --json` and
`channel set unstable --dry-run --json`, returned zero after installation.
Both report preview-only upgrade mode, no pending kernel activation and no
reboot request. The final selected channel remains unstable, the pending marker
is empty, the previous operation phase is complete, and the running kernel
remains 7.3.1-1500. This GUI delivery does not repeat package application,
partition, recovery, firmware or physical camera qualification operations.
