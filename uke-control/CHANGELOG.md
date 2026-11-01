# Uke Control changes

## 2.8.1 — 2026-11-01

Admit an independent single-device Btrfs maintenance journal with exact backing
device and remount verification. Import completed recovery creation checkpoints
only after checking their plan, both GPT copies, unit identity and exact current
layout. Retain the original checkpoint with the imported ownership record.
Report offline watchdog blockers in read-only layout status and require an
explicit inactive state in both offline layout and resize workers. Unknown or
unavailable state remains blocked. These preparation changes do not authorize
partition writes or establish physical F2FS conversion acceptance.

## 2.8.0 — 2026-11-01

Add a declared unstable package transition for the reviewed at-spi2-core,
GStreamer, libwacom and GNU runtime families, including installed development
children. Require a compatible core selection before enabling returns. Accept
only original Fedora candidates from admitted repositories with the owning
source, vendor, architecture, release and version floor. Retain all unrelated
removal, private-data and kernel protections. This release alone does not
withdraw any producer or install the replacement cohort.

## 2.7.1 — 2026-11-01

Merge the Control 2.7.0 qualified source with the independent stable, test and
unstable channel policy. Preserve the terminal client, microphone selector,
private firmware protection, retained kernel cohorts and interrupted-operation
receipts. Use the updater transaction interface for boot finalization so the
updater retains ownership of automatic policy and selected userdata profiles.
Keep driver interface 2 compression controls and their cached-state limits.
Native/package qualification and physical acceptance are separate checks.

## 2.7.0 — 2026-10-31

Add saved, event-driven microphone channel selection and bounded JSON status
to the CLI and bilingual TUI. Keep the kernel-owned raw four-channel source,
explicit targets and existing recordings. Default auto creates no additional
audio path. AEC/NS/AGC and uncalibrated automatic profiles remain unsupported;
CPU, energy and physical acceptance are not measured. See [MICROPHONE.md](MICROPHONE.md).

## 2.6.0 — 2026-10-30

Replace the three-pane TUI with task menus, explicit Back/Continue navigation
and full option explanations. Add terminal touch targets, drag scrolling,
screen keyboard/keypad and typed confirmation entry. Keep read ownership,
existing backend approvals and C++ client API v1. See [TUI.md](TUI.md) for
input compatibility and separate source, package and physical acceptance limits.

## 2.5.1 — 2026-10-30

Open the optional TUI when `uke-control` is entered in an interactive terminal.
Recognize Fedora's explicit numeric Rawhide prerelease identity and correct
root/nonroot trusted-layout fixture expectations. Release 2 was built as COPR
11232669 and installed through signed DNF transaction 239; that terminal check
did not execute a device control.

## 2.5.0 — 2026-10-30

Add the bilingual C++ terminal frontend and shared typed client catalog with
124 actions, plans, bounded read ownership and private receipts.
