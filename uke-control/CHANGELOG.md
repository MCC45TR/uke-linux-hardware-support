# Uke Control changes

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
