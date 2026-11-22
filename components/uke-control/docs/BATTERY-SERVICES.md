# Battery services in Control

11 November 2026, source and packaging scope for Fedora46/AArch64.

Control2.17.0-3 absorbs the unchanged battery-observer and offline-charge
payloads from battery1.4.1-7. The actual217r2 SRPM remains the cumulative
parent. Its52 non-spec sources, existing native checks, charge helper,
protection interval, settings and other runtime providers remain intact.
No new runtime package or hardware protocol is introduced.

The original battery and offline UI sources, native checks, units, D-Bus
policy, profiles, documentation and licenses are retained. Only the two
required executables are installed from that source; its older charge
helper and charge settings cannot overwrite the newer Control implementation.
Battery service idle waiting and demand activation remain unchanged.

The RPM pre-transaction captures an installed battery provider's activation
state in a bounded root-only volatile record. The old provider's removal
script may stop/disable it; the new Control post-transaction restores the
saved state, respecting explicit masks. Fresh installation uses the existing
sysusers/preset semantics. The isolated offline-charge UI is never started by
the ownership transition. The expanded RPM fragments are tested with fake
systemd before delivery. Normal and charger-boot hardware tests are separate.

The four original signed battery source/runtime recovery files are retained in
the existing [consolidation archive](https://github.com/MCC45TR/uke-linux-hardware-support/releases/tag/copr-consolidation-archive-20261101)
with `BATTERY-CONTROL-FOLD-SHA256SUMS-20261111`. Native build, signed payload
parity, DNF upgrade/fresh/reinstall/rollback and actual device ownership must
pass before removing the redundant battery producer. Existing charge and
offline physical qualification remains open; no power-limit change is made.
