# Separate the DSP filesystem service from sensor presentation

6 November2026, POCO Pad X1, Fedora AArch64. The native IIO consumer owns
sensor QMI transport and channel publication. Seven providers and their
standard attributes can be read directly on12206; this establishes transport
and unit sanity, without controlled pose, accuracy or lifecycle acceptance.

Filesystem service32 removes its unused SSC and dual-light SensorProxy
capability requirements. Its native daemon links libsystemd/libc/loader and
serves FastRPC independently of the frontend. Firmware/configuration paths,
read-only persist admission, private RAM registry, service ordering, scripts
and all native tests remain. Kernel/DT cannot replace the firmware file
protocol by parsing a private filesystem, so this service remains necessary.

The update extends complete source family1.0.0-3/COPR11239521 with both source
inventories verified. Only the required service and matching debug RPMs are
published; completed observation-client sources/tests remain historical inputs.
COPR11268501 succeeds85seconds from3ebef760. All three binary signatures,
exact output identities, the two-requirement delta,32 other installed
requirement providers, six unchanged data payloads and upgrade scripts/triggers
are independently admitted. ELFs remain AArch64 without SensorProxy/libssc.

Normal configured-repository DNF upgrades only the service from29to32 on the
tablet. All216 private RAM registry files remain byte-identical with the same
generation inode; persist stays read-only and all three service units are
active. Exact RPM/header verification, rawA+B, kernel trial/status and persistent
preferences/policies are preserved. After restart all seven native providers,
19 numeric attributes and the acceleration mount matrix read successfully;
repeated acceleration vectors change. Detailed calibration stays private.

Core meta and adaptive-display still require the installed presentation
frontend. Replacement with the original distribution SensorProxy requires
controlled axes, both light paths, proximity, startup/restart/suspend, dependency
closure and preserved user settings. No frontend/libssc is retired by this
update. No KDE/GNOME package is rebuilt; compilation stays in COPR and tablet
delivery uses DNF. Full sensor accuracy and suspend acceptance remain open.
