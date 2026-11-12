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
