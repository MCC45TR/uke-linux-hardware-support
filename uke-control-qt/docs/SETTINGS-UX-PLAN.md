# Qt settings redesign: topic pages and readable information tiles

Date: 4 November 2026. Status: proposal for review; no application implementation.

Implementation update: the user approved this plan. Qt 1.2.0-5 delivery and
its observed validation are recorded in the
[implementation lesson](../../docs/lessons/2026-11-04-qt-topic-settings.md).
The 129-action inventory below is the planning snapshot. The installed SDK later
exposed 134 actions; the map additionally assigns its five speech-processing
actions to the expert Sound tools. The runtime catalog is discovered rather
than limited to the original snapshot.

## Evidence and scope

The tablet currently has Qt 1.1.0-2, Control/client 2.8.1-23,
kernel updater 1.8.53 and battery observer 1.4.1-7. The running kernel observed
during this review is 7.3.1-7200-unstable. No update, slot write or reboot was
requested. Installed `actions --json` reports 129 actions in 16 SDK categories:
78 reads, 29 device requests, 14 local-file actions, four maintenance actions,
three package operations and one publication action.

The installed Qt design is represented by retained source 02536c1, including
navigation.cpp, AreaPage.qml, GuidePage.qml, RequestFields.qml, ObservationView.qml
and bridge.cpp. The workspace main branch still has an older Qt layout; a future
implementation must reconcile these sources deliberately. Unrelated battery
source changes in the workspace are outside this plan.

Only the Qt user experience is proposed here. GTK, CLI command names, hardware
writers, calibration, kernel selection and package publication are unchanged.
Any later native compilation belongs in COPR and installation goes through DNF.
New kernel development remains in the mainline-unstable family with an unstable
ABI. This redesign does not authorize stable-family publication or promotion.

## Problems found

1. The navigation follows commands rather than user tasks. Separate status,
   capability, admission, preset and application commands become separate guides.
2. Rows repeat descriptions, access labels, read/change classifications and
   numbered instructions before users reach a value or useful control.
3. A basic observation requires opening a guide, finding Read information and
   interpreting a generic flattened object. A read is presented like a workflow.
4. RequestFields chooses a combo box for every choice and a spin box for every
   integer. Semantic differences between booleans, real hardware steps, profiles,
   identifiers and exact limits are lost.
5. Parameter defaults are drafts, not current settings. They can look like actual
   settings until the user performs another explicit inspection.
6. Daily pages expose developer diagnostics, root recovery and overlapping
   observations alongside normal preferences.
7. Terminal handoff is disconnected from the page's state. Opening a terminal
   does not establish completion, successful application or physical effect.

The goal is fewer decisions and clearer values, not merely fewer category names.
The 38 feature groups in the action map are ownership groups, not 38 required
pages. Related groups share one topic page or an expandable detail section.

## Information architecture

Use six everyday category tiles in a vertical list. Put Help and Advanced in a
secondary group. Home is the settings list itself; remove the separate Overview
destination and the parallel everyday All actions entry. Expert search remains
available within Advanced. Global search returns topic destinations, with
technical actions behind an explicit expert filter.

| Main category | Topic pages/sections | Summary on the home tile |
| --- | --- | --- |
| Battery and charging | Battery use, charging preference, health, charger, sleep | Reported level and charging state, when available |
| Display and lights | Automatic brightness, rotation, flashlight, notification LED, external display | Actual automatic settings and connected display |
| Connections | USB accessories, NFC, Ethernet when present; links to original KDE networking | Connected accessory or link state |
| Sound | Audio devices, microphone preference; original KDE sound settings | Selected microphone or current audio availability |
| Storage and memory | Filesystem use, memory, advanced swap | Available storage and memory |
| System and updates | Updates, channel, kernel/boot, device information | Available/pending update state or installed version |
| Help, secondary | Collect evidence, saved snapshots, reviewed sharing | No background data collection |
| Advanced, secondary | Hardware diagnostics, wake permissions, storage recovery, expert reference | Explanation of its specialist purpose |

Camera and sensor availability belong to System > Device information. Camera
capture/focus tests and live sensor inspection are explicit diagnostic actions.
There is no ordinary camera setting in the installed catalog to justify a
dedicated top-level camera page. Availability must not imply full camera,
calibration or focus acceptance. Input diagnostics also belong to Advanced;
the catalog does not provide ordinary touch configuration.

Normal topics should be reachable in at most two navigational selections from
Home. Deep specialist tools can take a further step. Search can enter a topic
directly. A return retains page position; it must not resurrect a stale draft.

## Tiles and page structure

Home tiles have a theme icon, a short title, one actual-state summary and a
navigation arrow. The whole tile opens the destination. Do not put an additional
small settings button on it. Avoid instructional paragraphs in each tile.

Inside a topic, use grouped native FormCards containing related rows. A metric
row shows a readable label, value, unit and, only where needed, a brief validity
or age explanation. Do not create a separate large card for every tiny field.
On a wide page a few related metric tiles may use two columns; settings lists
remain one column. A wide window may show category navigation beside its page.
Portrait uses the native page stack and Back. Keep keyboard, touch and screen
reader access; no horizontal scrolling or truncated important values.

Conceptual Battery page; dashes are placeholders, not measured values:

```text
Battery and charging                           Refresh
[ Battery level       -- %    State unavailable       ]
[ Temperature         -- C    Latest valid observation]
[ Charging preference Current verified preference  > ]
[ Battery use         Session chart, when available  > ]
[ Battery health      Reported health and cycles     > ]
[ Charger             Reported source/protocol       > ]
  Technical details
```

Information appears directly on the topic page. Cheap, allowlisted passive
reads may start on entry, without authentication dialogs. Reuse bounded cached
results across tiles. A heavyweight slot-image check runs only in the kernel
topic or through explicit Refresh; never on a home timer. Firmware interrogation,
sensor acquisition, attended tests and privileged reads remain explicit.

Use a single page Refresh control, a readable loading state and an inline error
with Retry. Keep previous values visibly stale when a new read fails. Distinguish
not checked, loading, unavailable, unsupported, permission required and failed.
Unknown is never Off, zero or a healthy measurement. Read time and hardware
sample age are different: do not call a newly retrieved cache a fresh sample.

Only a blocked or pending condition warrants a prominent warning. Access labels,
schema names, action identifiers, raw output and provider details move to a
collapsed technical section. Keep genuine support limitations next to the
affected setting rather than as repeated warnings across the entire application.

## Per-topic necessity, controls and graphics

Here, "switch candidate" means a true immediate, acknowledged state change.
The current Qt SDK sends writes to a terminal; it does not provide this path.
In the first Qt-only revision such settings use a labelled choice and Apply,
not a switch that visually claims success before authentication or readback.
An immediate-control adapter is a separate reviewed prerequisite. It must keep
typed validation, existing authorization, per-action admission and real outcomes;
it cannot relabel writes as read queries or accept arbitrary commands.

| Topic | Necessary surface and placement | Preferred control | Graphic decision |
| --- | --- | --- | --- |
| Battery level/state | First Battery information tile | Read-only value | Simple level indicator; no extra gauge |
| Battery temperature | Battery summary/detail | Read-only value | Optional session line, not a separate default graph |
| Battery current/voltage | Expandable electrical details | Read-only signed values with units | Optional diagnostic lines |
| Health/cycles/capacity | One Battery health detail page | Read-only distinct reported quantities | No invented health score or ring |
| Cell topology | Technical health details | Read-only topology | No per-cell health chart without per-cell measurements |
| Charger/protocol | Charger detail page | Read-only reported source/contract | No graph; no inferred negotiated PPS/EPR version |
| Charging preference | One preference section in Battery | Supported presets; radio for a small set, selection dialog for a larger set; Preview/Apply | No graph required |
| End threshold | Same preference section; only if admitted | Exact numeric value with provider bounds, optionally supported presets | No unrestricted 1-100 slider merely because the CLI accepts that syntax |
| Charging support/admission/protection | Inline support state and reason | Read-only; technical expansion | No separate menu entry per helper command |
| Temporary full charge | Deferred; explanatory limitation in advanced details | No enabled timer/full-charge control | None; catalog provides preview only |
| Charge engineering/identity operations | Advanced expert tools | Explicit operation/file picker with original safeguards | No daily controls |
| Sleep/wake | Sleep summary in Battery; permissions in Advanced | Source list; deferred checkboxes and explicit Apply | No sleep timeline until event history exists |
| Automatic brightness | Display | Switch candidate; current implementation choice + Apply | None; this command is not manual brightness percentage |
| Automatic rotation | Display | Switch candidate; current implementation choice + Apply | None |
| Manual brightness/refresh mode | Original KDE page, if discoverable | Open original setting; a slider only with a real native provider | No fabricated Uke control |
| Flashlight | Display and lights | Off / low / high choice | No percentage slider: installed metadata says only two lit levels |
| Notification light | Display detail | Automatic/manual choice, native colour picker, supported intensity, short Test button | Colour preview only; no RGB hex text as the ordinary control |
| External display | Display section, present even when disconnected with a clear state | Connection summary; two named bandwidth/display profile choices when applicable | None |
| Lanes/DSC/VRR floor | Advanced display details | Named discrete choices or exact Hz input; explicit saved-versus-active state | No arbitrary continuous lane slider; next-boot changes are labelled |
| USB accessories | Connections | Device list with friendly identity; per-device policy choices | No topology graph in the everyday view |
| USB data role | Accessory details | Host/device/automatic options only as actually supported; explicit review if disconnecting | None |
| NFC | Connections | Switch candidate; otherwise choice + Apply | None; radio permission is not card-reading success |
| Ethernet | Connections; clear no-adapter state | Discovered-interface picker, automatic/supported link-profile choice | Link rate is a value; no traffic chart without counters/history |
| Wi-Fi/Bluetooth | Connections links when original KDE endpoints exist | Open original KDE networking pages | No duplicate Uke control inferred from diagnostic topic names |
| Serial/CDC | Advanced Connections | Separate session and persistent policy choices | None; expose connectivity effects |
| Sound/microphone | Sound | Original KDE audio settings; meaningful microphone labels and Automatic | No VU meter without an explicit capture provider/consent |
| Camera | Device information | Reported capability rows; Open original app only if a real app is available | No decorative preview or unsupported focus controls |
| Sensors | Device information; live values in Advanced | Friendly sensor list, explicit finite Start/Stop | A live line only for a selected numeric sensor with known units |
| Storage use | Storage and memory | Read-only volumes and used/available space | One horizontal used/free bar |
| Memory | Storage and memory | Available memory and clear cache semantics | Simple usage bar; live chart in diagnostics only |
| Swap | Advanced subsection of Memory | Named presets; exact MiB option only if supported; explicit Apply | No percentage slider or promise that more swap is faster |
| Updates | System and updates | Check, one reviewed package plan, Install | Phase/progress only when the backend reports it |
| Channel | Same updates page/detail | Stable/Test/Unstable radio list with effect descriptions and review | None; not a kernel-promotion button |
| Kernel/boot | Kernel topic | Current/next/accepted information; A/B cards with radio selection and Review | No graph; `next` becomes an Other slot shortcut that selects A or B in the draft |
| Incomplete update/recovery | Conditional banner linking to one recovery flow | Inspect/preview/recover under existing guards | No permanent Synchronize menu beside ordinary Update |
| Partition/root/resize/multiboot | Advanced storage maintenance | Discovered identifiers, inspection, one guarded wizard per operation | No decorative partition map before topology is trustworthy |
| Snapshots/logs | Help | Topic/component selector, native file dialogs, Collect | No graph or default full-system collection |
| Export/publication | Help sharing detail | Review local content, export; explicit separate publication | Never publish automatically after collection or failure |
| API reference and attended tests | Advanced | Searchable expert catalog and explicit test buttons | Only the appropriate test's actual measurements |

## Graph and data contract

An actual installed battery read returned provider_verified=true and
snapshot_coherent=true, with eight reading objects, while status remained
source_unqualified. Basic reported measurements and qualified health/ETA are
different questions. Treat validity per metric; do not suppress every reading
merely because the electrical/model profile is unqualified, and do not turn
those readings into a qualified health percentage or precise lifetime claim.

Priority one is a readable battery percentage-over-time history line with
charging intervals, when timestamps/state support them. Start only after valid
samples exist. Show actual coverage, sample units and gaps; never bridge suspend,
reboot, provider changes or missing data by extending the last value.
Temperature is optional. Signed current/voltage are diagnostic series. Do not
label battery-side current or calculated power as adapter input power.

The captured Uke battery API has no time-series endpoint or application energy
accounting. Observer checkpoints retain model accumulators, not a chart archive.
However, original UPower already provides a read-only GetHistory method. Real
inspection found HasHistory=false on the composite DisplayDevice but true on
the actual battery. A 24-hour charge-history request with resolution 96 returned
25 records: 13 with known states and 12 unknown-state markers, spanning 76,111
seconds. Their returned order was not ascending. These records do not prove
continuous 24-hour coverage or calibrated energy use.

First preference is a Qt read-only UPower history adapter using the discovered
actual battery object, its timestamps, values and states. Do not hardcode the
inspected path or treat empty composite history as proof that no history exists.
Order records by time, handle duplicate timestamps explicitly and preserve
unknown-state markers as breaks rather than plotting their placeholder values.
A one-hour/24-hour selector can request bounded history while showing its real
coverage and gaps. Do not duplicate UPower's recording service or history store.

If valid history is unavailable, offer a bounded in-memory session series while
the Battery page is active and the application foregrounded. Proposed cadence
is no faster than the observer's 30-second cache; choose 30-60 seconds after
measurement. Proposed cap is 720 samples per series, at most six hours at 30
seconds. Leaving the page/sleep stops fallback sampling and leaves a visible
gap. Before two valid points, show a collection state rather than a flat invented
line. Temperature/current plots need their own series; percentage history does
not supply those measurements.

No permanent recording service, sleep polling or new history store is part of
the first delivery. Consider a new durable store only if original UPower cannot
satisfy a documented requirement, with retention/privacy/storage limits reviewed.
Per-application battery consumption remains deferred until a real attribution
source exists; CPU load is not energy attribution.

Prefer the original distribution KQuickCharts for justified Qt plots. It is
installed on this device. Its presence proves dependency availability, not that
this application's chart renderer or data pipeline has been tested.

## Interaction and state rules

- A value is actual, requested or saved-for-later. Show the distinction where it
  matters; never initialize a control from an SDK default and call it current.
- A switch is reserved for a known binary state and acknowledged immediate
  operation. Busy prevents duplicate requests; failure restores the observed
  state and explains the outcome. Unknown remains a separate information state.
- A radio group suits two or three explained exclusive options; longer lists use
  a native selection dialog. A slider requires genuinely supported numeric
  variation. A spin box suits exact bounded values. Text is for actual free text,
  not discovered interfaces, UUIDs, input nodes, files or microphone identities.
- Updates, slots and destructive operations are button-driven reviewed flows.
  Current authentication, backend admission and required confirmations survive.
  The initial redesign must not silently remove SDK or boot-owner approval.
- Current terminal handoff can only report that a terminal was launched. Until a
  real receipt/readback exists, say that the result is unverified. Do not provide
  a fake completion percentage, applied switch state or successful-update toast.
- A page read coordinator deduplicates passive queries, pauses sampling during
  writes and serializes slot reads against boot-owner writes. Retry reads only
  under an explicit bounded policy; never automatically retry a mutation.
- Do not automatically run every metadata action tagged read: some acquire
  sensors, require privileges or open an attended terminal.
- Capability changes can invalidate a draft. Revalidate exact values and target
  identity immediately before application. Preserve null/false/zero distinctions.
- Existing theme, palette, icons, focus, accessibility names and text scaling
  remain native. Use Qt translation facilities consistently; the current English
  page strings must not coexist with partially translated Turkish settings.

## Delivery sequence and review gates

1. **Agree the structure and semantics.** Review this document, root tile names,
   the feature/action map, control choices and data limits. No implementation,
   package build or installation is part of this planning step.
2. **Build the topic shell in Qt.** Six root tiles, native topic pages, global
   search by user task, secondary Help/Advanced and merged information views.
   Retain a technical fallback for every installed SDK action. Do not change the
   shared GTK interface or deploy old boot sources from the retained Qt branch.
3. **Replace generic forms selectively.** Add feature-specific choices, discovered
   device/file pickers, readable availability and actual/requested/saved states.
   Keep current guarded terminal operations. Review the separate immediate-write
   prerequisite before introducing real switches.
4. **Add only useful visual data.** Storage/memory bars first; a bounded valid
   UPower battery-history graph next, with session fallback. New history services
   and per-app attribution remain outside this phase.
5. **Validate and deliver after implementation.** Compile/test in COPR; install
   through DNF. Test both narrow/wide layouts, portrait/landscape, scaling,
   Breeze/Adwaita, keyboard/touch/accessibility, all mapped actions, null/stale
   data, capability changes, authentication cancellation and partial failure.
   Test graph gaps and actual timestamps. Verify request/refusal independently
   of screenshots. Do not boot a known failed slot to claim a UX test passed.

Acceptance criteria: no command-shaped guide for basic status; no duplicate
ordinary status/settings destinations; no hidden root prompt on Home; no main
page with more than roughly six topic rows before a deliberate detail boundary;
no unsupported active-looking control; no false graph continuity; no default
expert console; no automatic firmware interrogation; no write reported complete
merely because a terminal opened. All 129 captured actions have exactly one
feature owner in SETTINGS-UX-ACTION-MAP.tsv; entry-point shortcuts share that
owner rather than duplicating the setting.

## Primary design references

- [Android settings guidance](https://source.android.com/docs/core/settings/settings-guidelines): readable titles, visible current values and intuitive topic grouping.
- [Android settings organization](https://developer.android.com/develop/ui/views/components/settings/organize-your-settings): grouped related preferences and subscreens.
- [KDE input guidance](https://develop.kde.org/hig/getting_input/): distinguish immediate switches, deferred choices, coarse sliders and exact numeric input.
- [Kirigami FormCard](https://api.kde.org/org-kde-kirigamiaddons-formcard-qmlmodule.html): original native grouped settings components.
- [KQuickCharts](https://api.kde.org/kquickcharts-index.html): original Qt Quick charts and data-source models.
- [UPower Device](https://upower.freedesktop.org/docs/Device.html): per-device history availability and timestamped history queries.
