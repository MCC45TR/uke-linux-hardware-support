# Uke Control terminal interface and shared client

Date: 2026-10-30. Software: Uke Control 2.5.0, native C++20 with ncursesw.

Start the optional `uke-control-tui` package as your normal session user:

```sh
uke-control tui
uke-control tui --language en
uke-control tui --read-only --start battery
uke-control tui --start charge.mode
```

Turkish is the default; F4 switches language in the running interface. A UTF-8
locale and interactive terminal are required. Minimum size: 64×18; 112 columns
opens the three-pane layout. Smaller windows show a resize prompt. `--no-color`
retains selection and warnings through text attributes.

## Navigation and option guides

| Key | Meaning |
| --- | --- |
| Tab / Shift-Tab | Move between categories, actions and guide |
| Arrows / j / k | Select an item or scroll the guide |
| Enter | Read an observation, open parameters, or choose an option |
| F1 / ? | Full help, execution behavior and limits |
| F2 | Show session favorites |
| F3 / slash | Search IDs, descriptions and parameter labels across categories |
| F4 | Switch Turkish / English |
| F5 | Open the related current-state observation |
| F6 | Open parameter form |
| F7 | Open the related preview, when defined |
| F8 | Validate parameters and review the request |
| Space | Mark a favorite; in an enum form open option descriptions |
| H | Bounded session history; Enter only views a past result |
| Esc | Cancel a pending read or return without executing a plan |
| q | Exit; an owned read is cancelled before exit |

Each category and action has an explanation. Parameter forms show purpose,
accepted numeric ranges, defaults, and descriptions for every enum option.
Defaults are requests, not current hardware readings. Left/right cycles enum
values; Enter opens the option guide. PgUp/PgDn scrolls a field's guide. Text/path
editing supports UTF-8, spaces, cursor movement, Home/End, Delete/Backspace and
Ctrl-U. Esc discards edits. Search matches Turkish case and common ASCII spellings.
Mouse clicks select panes/items, double clicks open an action, and the wheel
scrolls the focused pane.

| Category | Workflows |
| --- | --- |
| Overview | General status, capabilities, all components, hardware and workload |
| Battery | Health/provenance, status, cell topology and reported protocol |
| Charging | Admission, protections, protocols, policies, previews and review evidence |
| Display | Depth, lanes/profiles, DSC, current/saved VRR floor and color sensor |
| USB | Topology, finite watch, role, per-device power, CDC and transition recovery |
| Network | Exact Ethernet interface capabilities/profiles and network diagnostics |
| Desktop | Original desktop/KDE state, brightness and rotation policies |
| Lights | Flashlight and kernel LED auto/test/RGB requests |
| Sensors | Capabilities, single/finite samples and attended input diagnostics |
| Media | Audio/camera/NFC inventories and NFC permission |
| Sleep | Sleep observations and saved wake permissions |
| Storage | Inventory, exact root inspection, swap, resize and layout wizards |
| Packages | Channel selection/sync, signed previews, update and finalization |
| Recovery | Private package/USB pending state and original recovery routes |
| Diagnostics | All 19 topics, snapshots, comparison, safe export and report publication |
| Developer | Shared metadata, help/version and optional development tools |

Optional tools require `uke-control-tests`. Missing packages/providers remain
explicit errors. Enumeration, polling time and driver readback do not establish
physical acceptance, acquisition time or calibration.

## Results and execution

Read queries run asynchronously; Tab returns to browsing while a query remains
owned. One captured read runs at a time. Esc requests read cancellation; the
process group is terminated and reaped. Reads have per-action deadlines, 1 MiB
stdout and 256 KiB stderr limits. Truncation, timeout, cancellation and actual
exit status remain separate. stdout and stderr are never merged into JSON.

In a result, arrows/PgUp/PgDn/Home/End scroll, uppercase J toggles readable/raw
JSON, slash searches, and n finds the next match. Null stays unknown; zero,
negative readings and false remain distinct. Control, escape and directional
override characters are displayed visibly rather than interpreted by the terminal.

S saves a new private JSON receipt, mode 0600, without overwrite or following a
symlink. Receipts contain parameters, separate output/error, exit status and timing.
They can contain private paths and identifiers. Nothing is saved automatically.
History is limited to 32 entries / approximately 8 MiB retained payload and lasts
only for this process. It cannot replay a mutation.

Every mutation opens a separate review with the exact validated argument vector,
effect, parameters and access. Enter gives approval; package operations additionally
require APPLY/UYGULA, storage maintenance its action ID, and publication the exact
repository. These frontend confirmations retain the backend's admission and
interactive approvals. Previews are observations, not transaction reservations.

For foreground work the TUI restores the terminal and launches the fixed backend
command, without a shell. Root actions prepend `sudo --` to that command. Existing
active-session brokers retain their Polkit checks. The TUI adds no daemon, root
shell, broad policy rule or alternate storage writer. Foreground actions remain
owned until actual exit and have no TUI timeout. Ctrl-C is handled by the existing
command. Failed package outcomes require pending-receipt inspection; the TUI
never retries them automatically.

Diagnostics retain the current-directory output contract. Root-created logs and
reports remain root-private. Inspect the destination before approval. Publication
is a separate action with no default repository.

`--read-only` allows observations, parameter guides and plans but disables
mutations and receipt saves. Some observations contact providers, fetch package
metadata or request root access for protected state; none starts automatically on
selection. There is no background refresh while idle.

## Simulation and verification

From the source directory:

```sh
make -j4
LD_LIBRARY_PATH=build ./build/uke-control-tui --fixture tests/fixtures/tui.json
make check-client check-tui
```

Fixture schema: `org.uke.control.tui-fixture.v1`. `results` maps known action IDs
to `data` or `stdout`, optional `stderr`, exit `status` and bounded `delay_ms`.
Fixture mode visibly disables **all** process launches, sudo, hardware queries,
mutations and receipt saves. Missing results are explicitly unavailable. The
fixture is a bounded regular file and cannot define executable callbacks.

Client tests exercise catalog integrity, rejected inputs, literal argv, Unicode,
control handling, split streams, output limits, cancellation, descendants, real
read deadlines and private receipts. PTY tests drive the actual TUI through forms,
search, previews, results/history, resize and terminal restoration. These isolated
source/interface checks do not qualify physical hardware.

## Shared API for GTK and QML

`libuke-control-client.so.1` and `<uke-control/client.hpp>` contain no ncurses,
GTK, Qt, privileged backend or desktop dependency. Link C++20 clients using
`pkg-config --cflags --libs uke-control-client`. The catalog owns bilingual labels,
parameter types/choices/ranges, access, effects and inspection/preview references.

```cpp
#include <uke-control/client.hpp>
auto catalog = uke::client::catalog_json();
auto request = uke::client::plan("charge.mode", {{"mode", "firmware"}});
// Planning never executes or authorizes this request.
uke::client::Query query("battery.status");
// Poll from the frontend event loop; present query.result(); cancel on closure.
```

Query is single-owner; call poll/cancel/result on its event-loop thread. It accepts
only captured read actions, closes inherited descriptors, drains nonblocking
bounded pipes, and retains the process-group leader until completion. Integrators
must not independently reap its child. Destruction attempts bounded cleanup and
leaves a reaper for a killed uninterruptible child. Foreground `run_terminal`
requires a real terminal and explicit approval for mutations; a GTK/QML application
needs its own reviewed terminal/authentication adapter. Do not run a blocking
foreground writer on the GUI thread.

For clients that do not link C++ directly:

```sh
uke-control actions --json
uke-control actions describe charge.mode --json
uke-control actions plan charge.mode mode=firmware --json
```

Schemas: `org.uke.control.actions.v1`, `org.uke.control.action.v1`,
`org.uke.control.action-plan.v1`, `org.uke.control.client-receipt.v1`.
Metadata and plans do not access devices. Plans are revalidated from action ID and
typed values; caller-supplied argv is not trusted. There is no generic command
executor or automatic apply endpoint. Backend `libuke-control.so.0` ABI is unchanged.
Exported C++ layout changes require a client SONAME change; additive catalog entries
retain schema v1 and stable IDs. Consumers must handle unknown IDs/fields and
unavailable data.

Terminal lifecycle and wide input follow the official
[ncurses reference](https://invisible-island.net/ncurses/man/ncurses.3x.html) and
[wide input reference](https://invisible-island.net/ncurses/man/curs_get_wch.3x.html).
