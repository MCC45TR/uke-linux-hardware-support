# Uke Control terminal interface and shared client

Date: 2026-11-01. Software: Uke Control 2.8.0 source, native C++20 with ncursesw.

Install the optional `uke-control-tui` package and run as a normal session user:

```sh
uke-control
uke-control tui --language en
uke-control tui --read-only --start battery
uke-control tui --start charge.mode
```

With no operands, `uke-control` opens the TUI when both input and output are
terminals. Redirected invocations show CLI help; `--json` requires a command.
`uke-control --help` shows help without starting observations. Turkish is the
default; F4 switches language. A UTF-8 terminal is required. The minimum is
64×18; use at least 80×24 for comfortable touch input. Larger windows use taller
rows. `--no-color` preserves selection through text attributes.

## Categories and navigation

The interface uses one task list, a small command bar and separate action guides.
The layout is inspired by [bluetuith's menus](https://github.com/bluetuith-org/bluetuith);
no Go code, BlueZ client or other application is embedded. The C++ client retains
all existing action IDs and API v1. UI menus are independent of API categories.

| Category | Contents |
| --- | --- |
| Device status | Selected battery, device and workload observations |
| Power & battery | Battery readings/health, charging preference/limit and wake permissions |
| Display & lights | Original KDE brightness/rotation controls, flashlight and LED |
| Connections | USB/Ethernet observations and NFC permission |
| Audio & sensors | Microphone channel, audio/camera observations and sensor readings |
| Updates | Current channel, update preview and installation |
| Advanced | Diagnostics, expert controls, storage, recovery and developer tests |

All seven areas fit at 80×24. Daily pages contain an explicit set of reviewed
tasks. Newly added catalog entries stay in the appropriate advanced area until
their daily workflow is reviewed. Expert charging admission/setup, display
lanes/DSC/VRR, USB roles/serial export and optional input tests are kept there.
Storage and recovery are separate advanced pages; boot/partition maintenance
adds a data-loss/restart warning and retains typed and backend confirmations.

Wi-Fi/Bluetooth shortcuts under Advanced → Diagnostics & support preselect the
existing diagnostic topic. They do not
scan, discover, pair, connect or introduce new radio controls. Reports retain their
current-directory file contract and a separate review before creation.

Select an area and an action. On daily pages, selecting a parameter-free captured
observation reads its actual provider directly. Other actions open their guide.
Continue opens parameters
or a separate request review; Read starts an explicitly selected observation.
Back restores the previous screen, selection and scroll position. Home
returns to the root menu. Back at the root does not exit; the visible Quit button
or q exits. No device observation starts from highlighting or opening a guide.

| Key | Meaning |
| --- | --- |
| Up/down, j/k, Tab/Shift-Tab | Select a row or scroll text |
| Enter / right arrow | Open, edit, choose or advance as the visible button indicates |
| Esc / left arrow | Return without executing or cancelling a read |
| Home | Return to Categories |
| Page Up/Down | Move through long lists or explanations |
| F1 / ? | Help |
| F2 / Space | Open session favorites / mark the action guide |
| F3 / slash | Search all actions; slash finds text when viewing a result |
| F4 | Switch Turkish / English |
| F5 / F7 | Open related current-state / preview guide, when defined |
| F8 | Validate a form and review its request |
| F9 | Reopen the running read |
| R on a completed captured read | Refresh with the same validated parameters |
| D / C in review | Show exact arguments / edit required confirmation |
| H | View bounded session history |
| q | Quit outside text entry; stop an owned read first |

## Touch and parameter input

Every visible button and list row accepts a single tap. Touch uses the terminal's
mouse protocol, without reading input devices. Konsole/xterm-compatible SGR mouse
reporting is required; a terminal that consumes touch gestures must forward them
as application mouse events. Font size determines the physical target size.

Vertical dragging or the large up/down buttons scroll lists and explanations;
the wheel also works. A drag release does not activate a row. Press/release must
refer to the same target and screen. Navigation invalidates pending presses and
briefly rejects a second tap, preventing click-through into the next screen.
Resize also invalidates a pending gesture. Keyboard input remains independent.

Tap a parameter to edit it. Choice rows open their full, scrollable explanation;
Choose accepts, and Back preserves the earlier value. Forms show numeric ranges
and current draft values. Defaults are requests, not current device readings.
All values use shared client validation before planning or execution.

Text/path fields accept UTF-8, spaces, cursor movement, Home/End, Delete/Backspace
and Ctrl-U. Esc discards the edit. Touch entry opens a built-in Turkish/English
keyboard with case, symbols, space, cursor, erase and clear buttons. Numeric entry
adds a keypad and bounded ±1/±10 controls. Hide Keyboard reveals the full field
explanation; Keyboard restores touch entry. This also works for search, receipt
paths and typed confirmations. Search matches Turkish case and common ASCII
spellings. Accepting confirmation text returns to review; Run is a separate step.

System passwords and the existing foreground backend questions use the terminal
or system input method. The built-in keyboard is limited to TUI forms; it does not
intercept credentials or replace those backend dialogs.

## Results and execution

Captured reads run asynchronously and remain owned until actual process exit.
Back leaves the read running. The Reading/F9 button reopens its result; Stop or x
on that result requests cancellation. Only one read runs at a time. Writes restore
the normal terminal and have no TUI timeout. No periodic device polling is added.

Results distinguish completion, nonzero exit, cancellation, timeout and partial
output. Empty output has an explicit message. Failed reads keep provider details
and offer an explicit refresh. Refresh replaces the current result page, retains
the old receipt in session history and never repeats a mutation or foreground
operation. Related buttons appear only when an observation/preview exists.
J toggles readable/raw JSON; slash searches output and n finds the next
match. Unknown/null is distinct from zero, negative numbers and false. Control,
escape and directional override characters are rendered visibly.

Reads retain per-action deadlines, 1 MiB stdout and 256 KiB stderr limits. Streams
are kept separate. S saves a new private 0600 JSON receipt without overwrite or
symlink following. Receipts may contain private observations and paths; review
before sharing. History stays in memory, bounded to 32 entries / 8 MiB of retained
history output. Opening history never replays a command.

Every mutation requires a separate review and Run action. Details/D shows access
and the exact validated argument vector, without a shell. Packages additionally
require APPLY/UYGULA, maintenance its action ID, and publication the exact repository.
The confirmation field is editable by tap or C; accepting text does not run a plan.
Existing backend admission, freshness and interactive confirmations remain required.
Previews are observations, not reservations. Root actions hand over only their
validated command to `sudo --`. No root TUI, daemon, broad policy or alternate
storage writer is introduced. Failed package operations are never retried silently.

`--read-only` permits guides and observations but disables changes and receipt
saves. Some observations contact providers, fetch metadata or need protected-state
access. Missing packages/providers remain explicit errors. Driver readback and a
command's exit status do not establish physical hardware support.

## Simulation and verification

From the component source directory:

```sh
make -j4
LD_LIBRARY_PATH=build ./build/uke-control-tui --fixture tests/fixtures/tui.json
make check-client check-tui
```

Fixture schema: `org.uke.control.tui-fixture.v1`. `results` maps known action IDs
to `data` or `stdout`, optional `stderr`, exit `status` and bounded `delay_ms`.
Fixture mode disables every command, sudo, device query, mutation and receipt
save. Missing results are unavailable. A fixture must be a bounded regular file;
it cannot supply executable callbacks.

Native PTY/libvterm checks cover all 126 action placements, daily/advanced
separation, direct daily reads, repeated refresh/back, empty/error results,
maintenance confirmation refusal, keyboard navigation, SGR taps,
dragging, resize invalidation, forms, numeric bounds, typed confirmation, Unicode,
search, history, cancellation and terminal restoration. Xvfb/xterm screenshots are
synthetic visual evidence. Neither establishes physical finger-touch acceptance,
backend control success or hardware qualification on POCO Pad X1.

## Shared API for GTK and Qt

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
requires a real terminal and explicit approval for mutations. The native
[GTK](../uke-control-gtk/README.md) and [Qt](../uke-control-qt/README.md) applications
reuse the header-only `<uke-control/frontend.hpp>` adapter after native review.
It revalidates action ID/values and opens an original distribution terminal with
the fixed ordinary-user `frontend-request` companion. The companion revalidates
the request again and uses the existing `run_terminal` authentication path, which
adds `sudo --` only for root backend actions. It keeps the actual command exit
status visible until Enter closes the terminal. No shell, new D-Bus
method or privilege policy is added. Its `launched` result means only that the
terminal process started; the backend operation may still fail or remain pending.
The applications ask users to inspect that terminal and current state afterward.
They do not automatically replay requests or kill foreground work when closing.
Build consumers with thread support. Do not run foreground writers on GUI threads.

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
