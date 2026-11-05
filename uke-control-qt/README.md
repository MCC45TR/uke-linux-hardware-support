# Uke Control for Qt

A native Qt 6 Kirigami frontend for the existing typed Uke Control client.
Application logic and the device adapter are C++; presentation uses original
Kirigami/FormCard controls, the desktop palette and inherited theme icons.
GTK and the existing CLI/device writers are separate components.

## Android applications

Open **Android applications** from Settings or System and updates. Choose
**GAPPS — Google Play included** or **Vanilla — without Google services**, then
select **Install Waydroid**. KDE requests administrator authorization. The
native installer uses original Fedora DNF/Waydroid packages and official OTA
channels, checks complete image files, applies the packaged Uke graphics/window
defaults and enables the original container service. **Open Waydroid** then
starts Android as the normal desktop user. The `--waydroid` application option
opens this page without starting installation.

A fixed system service owns installation, so closing the interface does not
interrupt package installation or downloads. Returning reads persisted progress.
The service holds a shutdown/sleep inhibitor while it runs. Failed/interrupted
attempts can be retried. Only GAPPS/Vanilla and fixed operations are accepted;
there are no arbitrary command or image-URL inputs. A complete existing install
is reused, a different edition is preserved, and custom image locations are not
silently replaced. No application data is erased. An unsupported running kernel
requires a normal system update/restart; the installer never changes boot policy.

Google sign-in happens inside Android. If registration is required, the page
reads the GSF identifier through a separate authorized read and opens Google's
official registration page. The identifier is excluded from progress files/logs
and cleared on leaving the page. Account completion and game compatibility are
separate from successful installation. Original Fedora Waydroid retains its
upstream Python helpers under the existing distribution-package exception; no
project Python or on-device SDK/compiler is added.

## Everyday settings

Home contains six category tiles: Battery and charging, Display and lights,
Connections, Sound, Storage and memory, and System and updates. Help and
Advanced are secondary destinations. Ctrl+K searches topics, including English
and Turkish aliases. The complete installed SDK catalog remains searchable in
Advanced. Back retains the category/home path and cancels owned observations.

Topic pages show related information directly, with readable labels, units,
availability and read times. Secondary measurements expand on request. The
battery, charging and software interfaces keep reported, requested and saved
values separate. Missing data is never interpreted as zero, Off or acceptance.
Unsupported charging requests and inconsistent boot metadata disable the daily
change entry; the expert backend retains its independent safeguards.

Preferences are edited within their topic and then reviewed. Small choice sets
use radio buttons, longer sets use native selection dialogs, and exact numeric
requests use bounded spin boxes. The flashlight offers its actual Off/Low/High
steps. File, folder and colour requests use native pickers. Available network,
USB and input identities are discovered without asking users to invent node
names. Original KDE settings modules are linked only when installed; their
applications and packages are not copied or forked.

## Battery history

The page discovers the actual power-supply battery through original UPower,
checks history support and reads bounded existing charge history asynchronously.
It does not assume that the composite DisplayDevice has history. Records use
actual timestamps and percentage/state values. Unknown-state markers, duplicate
timestamps and long uncovered spans break the line. Values outside the requested
window, malformed samples and invalid percentages are omitted; memory is bounded.

The native Qt plot uses the desktop palette and font, with a fixed percentage
axis and actual time coordinates. It does not resample missing periods into
zero or imply continuous coverage. No additional recording service, sleep poll,
history database or per-application energy estimate is added.

## Reads and reviewed changes

Only fixed, allowlisted topic observations run on entry. Queries are read-only,
bounded, serialized and cancellable; cached observations are shared with home
summaries. Sensor acquisition, firmware interrogation and attended tests remain
explicit expert actions. Raw provider output remains in the expert view.

The C++ adapter validates typed IDs and values before every request. Reviews
retain the frozen request, separate consent and required confirmation tokens.
Typed device actions retain the frontend-request terminal path, authorization
and backend qualification. Dedicated Kernel Manager and Waydroid pages use
their fixed native authorized helpers. There are no optimistic instant switches.
A terminal opening is not reported as operation completion, verified application
or physical hardware success. The GUI never automatically retries a mutation
or terminates an independently handed-off terminal operation when closed.

Run as the normal graphical-session user. A root GUI is refused. Simulation
uses the bounded org.uke.control.tui-fixture.v1 file, never falls through to real
queries and blocks all real mutations, native settings launches and handoffs.
Read-only mode independently refuses writes. Control characters remain escaped.

## Packaging and validation

On POCO Pad X1, all native compilation belongs in COPR. Install the application
and optional tests through DNF. Do not compile on the tablet or install copied
binaries. The RPM requires the matched Control SDK/frontend capabilities and
original Fedora Qt/KDE packages, including Kirigami Addons 1.9 or newer for the
native file/folder delegates. Turkish messages use the application's KI18n
translation domain.

The optional uke-control-gui-qt-tests RPM provides:

```sh
/usr/libexec/uke-control-qt/tests/run-installed.sh
```

The native suites cover every catalog guide, all topics, typed validation,
discrete controls, history ordering/gaps, zero/negative/unknown values, review
refusal, cancellation, theme icons and real QML scene placement. Set
UKE_QT_TEST_SCREENSHOTS to a private directory for window-only fixture captures.
Opt-in UKE_QT_TEST_DEVICE_READS=1 additionally reads installed providers and
existing UPower history. Device tests preserve incomplete reported boot evidence
rather than requiring a trial kernel to be accepted.

Source builds use CMake, Qt Widgets/Qml/Quick/QuickControls2/DBus/Network,
KI18n and the existing pkg-config uke-control-client SDK. No Python runtime or
privileged GUI is introduced. Fixture/source checks are not physical acceptance
of every hardware provider, charge cutoff, camera focus or a boot transition.

The USB and dock page groups actual kernel accessory link speeds, runtime power,
USB Ethernet counters, Type-C data/power roles and advertised display modes.
Automatic connection handling is owned by UCSI; Host/Device requests are single
swaps rather than a saved forced mode. The charge page reads verified firmware
settings separately from write admission and offers the existing guarded limit
request when admitted. A fixed read-only PolicyKit helper handles protected
charging information and the fitted NFC tag's capability query.

The built-in FM19511 is a Type 2 tag, not an NFC reader. Explicit external NDEF
reading uses the original Qt NFC backend, with a 30-second deadline and local
plain-text presentation. A supported external reader is required. No EEPROM
contents are read or written by the built-in tag information page.

Senemos Kernel Manager combines live CPU/GPU observations with advertised
frequency limits and governor controls. Graphs are bounded, pause off-page and
show missing data as gaps; they do not record a background history. CPU load is
computed from kernel counters. GPU frequency is not a GPU utilization estimate.
The sensor page reuses the native sensor owner and preserves unknown sample age.
A reviewed single-attribute frequency request requires administrator authority,
checks the expected state and reads the value back. It never changes boot
configuration, voltage or thermal protection. Memory/swap and sleep/wake facts
and controls are presented together.
