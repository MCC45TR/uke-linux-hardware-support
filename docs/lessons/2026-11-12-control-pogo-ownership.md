# UKE-CONTROL-POGO-20261112: consolidate helpers with original Fedora QCOMTEE

Date: 12 November 2026. Environment: POCO Pad X1, Fedora 46 AArch64,
Linux 7.3.3-24-unstable and installed Control 2.17.0-12.

The installed pogo-auth4 read-only firmware check rejects the running 7.3.3
family before inspecting firmware. Its accepted source admits only 7.3.1 and
waits with a one-second poll plus a fixed sleep. Control13 corrects only the
qualified family check and waits for a kernel request or a signal eventfd.
The board, driver, TA hash, challenge, token and secure-world admission gates
remain intact. New native fixtures cover a stop arriving before poll, a stop
while blocked, request/removal events and restoration of signal handlers.

The candidate folds the two existing standalone helpers into Control. The
original desktop helper C++ remains unchanged; its unit attachment condition
moves to the native auth helper, so system authentication needs no Qt.
Desktop ELF dependencies remain conditional on original PowerDevil. Protected
RPM receipts preserve system and online user states across ownership changes,
upgrades and reinstalls. Private command-interposer fixtures passed for active,
inactive, masked, runtime-enabled, failed-restoration and fresh-install cases.
Inherited SSC state, refresh and cache-removal fixtures also passed locally.
These are data-only tests; no native compilation ran on the tablet.

The signed original Fedora quic-teec1.0.2-3 source has identical public headers
and object/memory implementations to the accepted Qualcomm pin. The candidate
links that distribution library and its libcbor dependency rather than
rebuilding QCOMTEE/QCBOR. Its real credentials callback has a dedicated native
map/error test without TEE access. The old QCBOR failure-injection fixture is
retained for source regression only; its encoder is absent from the runtime.
The original MINK adapter remains necessary because Fedora provides its IDL
compiler but not the adapter library. A direct client migration needs separate
object, callback and lifetime parity, not a replacement authentication protocol.

Both exact accepted source RPMs and signed standalone runtime RPMs, with a
manifest and hashes, were archived and verified by public digest/size and
streaming SHA256 readback. No standalone producer has been retired yet.
Native COPR qualification, signed normal DNF upgrade, fresh profile closure,
reinstall and rollback are pending. Physical attachment, secure-world
authentication, keyboard input, lights and suspend remain unverified without
the accessory. No TEE operation, firmware write, kernel selection or reboot
was performed during preparation.

The source audit of candidate13/COPR11316177 found a root-unit uninstall gap:
only the desktop helper had standard removal hooks, also absent in the old
auth4 recipe. Control14 adds the corresponding systemd lifecycle macros.
Candidate13 remains uninstalled; its live build is observed to completion,
without cancellation, replay or producer retirement. Native and delivery
acceptance for the corrected candidate remain open.

Before submitting14, the rollback review also finds that the accepted old
standalone helpers have no automatic live-process refresh on installation.
The candidate now captures a missing receipt in preun and restores it in
postun while a package instance remains. It accepts only known owners during
RPM replacement, preserves an earlier receipt and never restores on complete
removal. New data-only cases exercise the actual shared fragment with mixed
owners, rollback, preexisting state and removal. All retained state fixtures
pass. This source preparation remains distinct from a real DNF rollback.
