# Command-line interface

30 October 2026, Control 2.5: `uke-control tui` opens the optional guided C++
terminal interface. `actions [describe ID|plan ID KEY=VALUE...] --json` exposes
the same versioned bilingual catalog and validated plans without device access
or execution. See [the terminal and frontend API guide](TUI.md).

25 October 2026. Control 2.0 defaults to readable terminal output, including when
stdout is redirected or piped. Structured output requires an explicit `--json`.
This is a major CLI compatibility change: scripts that consumed default JSON
must add that option. Existing backend/D-Bus schemas remain structured.
Source and host validation are separate from native RPM and tablet acceptance.

```sh
uke-control --help
uke-control charge --help
uke-control --version
uke-control status
uke-control flashlight status
uke-control flashlight set 25
uke-control usb status
uke-control channel status
sudo uke-control channel set test --dry-run
sudo uke-control cdc on --persistent
sudo uke-control usb role host
uke-control diagnostics wifi
uke-control --json usb status | jq '.typec'
uke-control charge plan charge80 --json
```

The human view names fields, includes units, distinguishes unavailable values
from false, and omits schema bookkeeping. It never interprets missing evidence
as a successful hardware test. Terminal control bytes in backend responses are
escaped. Private report files retain their original evidence formats and paths;
readable terminal output does not redact their contents.

`-h` and `--help` show the command group's syntax without running its operation.
`--version` also exits before dispatch. Global options can appear before or
after the command; `--` ends global-option processing. Invalid commands and
usage report a diagnostic on stderr and return 64. Operational failures remain
nonzero, including an unavailable actuator's existing 69 status. Successful
results go to stdout. The CLI uses no automatic pager or terminal color in its
static output.

| Preferred command | Accepted compatibility spelling |
|---|---|
| `channel status` | `branch status`, `brunch status` |
| `channel set stable` | `branch stable`, `brunch stable`, `channel stable` |
| `channel sync` | `branch sync`, `brunch sync` |
| `cdc on` / `cdc off` | `set cdc on` / `set cdc off` |
| `usb role host` / `usb role device` | `usb request data host` / `usb request data device` |
| `diagnostics TOPIC` | `diagnostic TOPIC` |

Existing privilege, qualification, confirmation and rollback checks still apply.
`--json` never grants permission and never implies `--yes`. Package changes with
JSON require explicit `--yes`; `--dry-run` or update's `--check` remains a preview.
Package status, previews and successful transactions have a single structured
result. With `--json`, CLI and package exception paths emit
`org.uke.control.error.v1` on stderr, with a stable code and nonzero exit status.
Helper/measurement streams retain their documented contracts; callers must check
status before parsing stdout. Private helper logs and
kernel-updater progress use stderr when package JSON is selected.

`usb watch --json` and `sensors -n5 --json` emit one complete object per sample.
The original structured query schemas and numeric values are preserved. Plain
file-path receipts use an `org.uke.control.result.v1` object with an `output`
string. The general `status --json` now has one `org.uke.control.status.v1`
object containing swap, charge and USB sections. `swap status --json` has its
own `org.uke.control.swap.v1` object and does not query charging or USB.

Interactive resize preparation accepts human output only; `resize system
--check --json` is a read-only geometry result after the existing admission
checks. GitHub report submission prints its issue URL and rejects `--json`
before publishing. Development `test desktop` and `test hardware` accept JSON;
display and cadence measurement streams offer explicit `--csv`. Cadence's
normal output labels each frame and timing unit. The native resize helper gains
no new library dependency in the shutdown environment.

The widget explicitly requests JSON for every backend command. Control 2.0
conflicts with widget revisions older than 23, so a package resolver cannot
silently combine the new CLI with a client expecting default JSON. Widget 23
requires Control 2.0. The widget remains separately removable and KDE-specific.

30 October 2026: Control 2.4 adds `capabilities`, `complete [PREFIX]`,
`overview [COMPONENT]`, `overview save FILE [COMPONENT]`,
`overview --fixture FILE`, `compare BEFORE AFTER`, `support export INPUT NEWFILE`
and `roots list|inspect PARTUUID`. See [the bounded read-only API](OVERVIEW.md).
Completion metadata and help share one command registry. Bash completion reads
that metadata and never runs an actuator. `usb transition status` reports the
current-boot inhibit receipt; root-only `usb transition recover` clears a receipt
only after stable, verified disconnected state. It does not change a USB role.
The existing module-loader hook requests and strictly validates the one-field
JSON minimum-frequency response; it adds no general parser or Python dependency.

The option conventions follow the [GNU command-line guidance](https://www.gnu.org/prep/standards/html_node/Command_002dLine-Interfaces.html).
The renderer uses the existing [libsystemd JSON API](https://www.freedesktop.org/software/systemd/man/262/sd_json_parse.html).

`sudo uke-control singleboot` and `sudo uke-control multiboot` open the explicit
storage wizard. Both support a private metadata `--dry-run` and a read-only
`status` query (`--json` optional). See [boot layout conversion](BOOT-LAYOUT.md)
for the original-userdata boundary, ownership evidence and two-approval data-loss
policy. Neither command installs an operating system.

26 October 2026: Control 2.2 adds these reports and guarded controls.

| Command | Meaning |
| --- | --- |
| `uke-control battery status` | A bounded read-only snapshot from the existing battery observer. |
| `uke-control battery health` | Reported pack health, vendor SOH, cycles and qualified retention with provenance. |
| `uke-control battery cells` | Reported topology and individual-cell health availability. |
| `uke-control charge review /path/to/review.json` | Install root-reviewed, expiring identity evidence for the current boot/source; no service activation or charging SET. |
| `uke-control charge restore-current` | Stop the automatic policy after admission, then restore and read back the profile's reviewed positive FCC value. This is not a factory reset. |
| `uke-control battery protocol` | Cached USB supply class and actually reported negotiated protocol/revision. |
| `uke-control wake status double-tap` | Wake permission and the NT36532 gesture-armed state. |
| `sudo uke-control wake set double-tap on` | Save supported wake permission; use `off` to disable it. |
| `uke-control nfc status` | Linux NFC controllers, rfkill state and configured FM19511 tag interfaces. |
| `sudo uke-control nfc on` / `off` | Unblock/block one uniquely identified NFC radio. No polling or tag-memory writing. |

Readable default output and explicit `--json` are retained. Missing individual-cell
telemetry produces an unavailable result; pack health and equal-share estimates
are not individual measurements. A reported USB class does not identify a PD
revision, QC version or verified MI session. The protocol report uses the existing
cache-only charge query. Other battery views reuse the read-only observer and may
read its standard power-supply attributes. An incoherent snapshot is not promoted
to usable health data.

The Uke OEM DT at `94c84aeb9e517dbde546fc732033c184094c8e57`,
`qcom/uke-sm8635.dtsi`, defines an `fm,fm19511` NFC tag. The manufacturer's
[POCO Pad X1 FAQ](https://www.mi.com/my/support/faq/details/KA-628084/) separately
excludes card reader/writer and card emulation while advertising tap-to-share.
NFC status reports exported Linux interfaces; DT configuration and declarations
do not establish physical operation. Radio changes require exact Uke/SM7675,
one associated NFC rfkill switch, no hardware block when enabling, and permission
readback. Unrelated radios are excluded.

Double-tap requires the bound `nt36532-uke-thp` driver, its explicit
`gesture_wake_armed` interface and a valid wake permission. Saved permission and
currently armed state are separate. The client adds no panel power policy,
gesture packets or firmware commands. Actual suspend/wake requires tablet testing.
