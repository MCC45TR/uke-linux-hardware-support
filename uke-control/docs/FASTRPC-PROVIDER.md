# Uke FastRPC integration with original Fedora packages

Control supplies session lifecycle, notification and private file namespaces.
The native kernel driver owns FastRPC resources and original Fedora fastrpc
owns the userspace protocol. Control does not ship a modified FastRPC library
or the legacy HexagonRPC daemon. Its existing cache helper, session starter
and readiness adapter require no supplementary runtime RPM.

The root and sensor services retain their existing names and consumer ordering.
They use a static private RootDirectory with the original loader, system
libraries and Control helpers bound read-only. Configuration, plugin and cached
SoC attributes are read-only; the complete disposable registry parent is the
only writable data tree. Fixed internal aliases preserve the OEM paths and
cross-alias atomic rename. Factory persist is never bound into the sessions.
Proc/sys are empty except cached SoC attributes. Only ADSP FastRPC and the
system DMA heap are admitted. The unrelated Fedora fastrpc-services audio/CDSP
autostart package is unnecessary.

A dedicated16MiB noswap tmpfs stores the active cache. Before initial migration,
the cache helper takes exclusive locks on both old and new cache readers and
copies the accepted RAM generation. It rejects unsafe directories, incomplete
generations, symlinks and active readers. Existing registry changes and parent
metadata survive migration; subsequent warm generations refresh configuration
atomically and retain their own state. The accepted old RAM generation remains
available during rollback qualification. No disk backup or factory write occurs.
Cache/provider stop-start cycles retain the mount and warm state.

The outgoing RPM's postun checks whether the incoming package still supplies
the private mount unit. Retained-provider upgrades/reinstalls leave warm RAM
alone. Downgrade to accepted Control6 or removal stops the old readers/cache
and unmounts in one systemd transaction; incoming Control6's pre/posttrans
snapshot preserves the five consumer states and enable/mask preferences. An
offline image transaction does not contact systemd. A later upgrade can import
the accepted provider's current RAM generation instead of reusing stale RAM
from its previous installation. No factory input or accepted old generation is
removed, and no executable cleanup helper is added to the runtime payload.

The cache unit verifies read-only factory input and the actual tmpfs/noswap mount
before running. Private session processes have no capabilities, AF_UNIX only,
MDWE, zero permitted swap and no core dumps. SDK stdout/stderr is disabled to
keep private inputs out of logs. A completed listener exits; systemd releases
dependencies and applies the existing restart delay. Enable and mask preferences
remain separate from active state and are preserved through supported DNF
transactions. The Adaptive target is restored only if it was previously active,
and a newly applied explicit mask takes precedence.

Both sessions explicitly set BindLogSockets=no. Original systemd262.2 otherwise
implies journal socket binding for PrivateDevices with RootDirectory, even with
MountAPIVFS=no and null standard output. The installed usr_t static root rejects
init_t creation of missing journal mount targets under Enforcing SELinux. The
sessions need only the explicit notification socket; no journal sockets or
SELinux policy relaxation are required. Source: systemd v262.2
src/core/execute.c:exec_context_get_effective_bind_log_sockets and
src/core/namespace.c:bind_log_sockets_table.

The target software fixture uses the actual static-tree staging and usr_t label,
substitutes unmodified Fedora sleep at the SDK executable mount target, and
keeps production services live. The implicit-binding baseline fails226; explicit
no starts, with journal sockets absent, notify available and the existing cache
and service states retained. A separate bounded real-listener trial uses the
same static label, observes stationary outputs and fresh restart PIDs, then
automatically restores accepted Control6. Both fixtures remain source tests;
neither installs a local binary or proves cold boot or suspend acceptance.

Bounded tests on kernel24 with Control6's existing helpers and original Fedora
fastrpc1.0.8-1 verify both actual ready sessions, stationary accelerometer/gyro
and dual-light shapes/status, fresh PIDs after a root restart, proxy presence,
ten-second idle CPU delta0/0, swap0 and unchanged DSP crash counts. Every trial
returns to the accepted provider and removes its temporary resources. These
prerequisites do not establish sensor accuracy, magnetic heading, proximity,
long-duration stability, DSP crash recovery or suspend/resume. Source/native
qualification and signed RPM/DNF upgrade/reinstall/rollback remain separate
from hardware acceptance; consult the canonical migration evidence.

The cache derives from the SSC32 GPL-3.0-or-later project helper; its original
source is preserved unchanged in the cumulative sources. Session/notification
integration is MIT; original Qualcomm FastRPC is BSD-3-Clause. Calibration is
device-private RAM input and is never included in an RPM or source archive.
