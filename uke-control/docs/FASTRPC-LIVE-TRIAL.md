# Original Fedora FastRPC live qualification

12 November 2026, Uke, Fedora46/AArch64, kernel7.3.3-24-unstable,
Control2.17.0-6 and original Fedora fastrpc1.0.8-1. This is a bounded trial,
not a persistent service or RPM transition.

The existing Control session and notification helpers load the unchanged,
versioned Fedora listener and libadsprpc. Both real root and sensor sessions
reach Type=notify readiness. Process executable and mapped library identities
are checked. The optional fastrpc-services package is absent; its unrelated
audio/CDSP autostart rules are not needed.

A private16MiB noswap tmpfs holds a small copy of the accepted cache. Its
registry parent has one writable bind with a fixed persist alias, preserving
atomic cross-alias rename. Config, DSP plugin and cached SoC attributes are
read-only. Factory persist is not mounted inside the trial root. Proc/sys are
empty except the cached SoC subtree. Access is limited to the required ADSP
FastRPC and system DMA-heap devices. SELinux remains Enforcing; capabilities
are empty, address families are AF_UNIX only and MDWE remains enabled. Both
service cgroups have MemorySwapMax=0; core dumps and SDK output are disabled.
No calibration bytes or raw sample values are retained in trial records.

The independent recovery timer is first qualified with a public dummy unit.
The real trial arms a75-second recovery timer and an immediate exit trap
before stopping consumers. It changes only uniquely named runtime drop-ins.
The old cache remains the accepted generation. Every result restores the
legacy Control provider, saved active/enable states, and removes the trial
mount, namespace tree and owned drop-ins. A newly applied explicit mask wins.
No package transaction, kernel selection, reboot or native tablet build occurs.

The first live attempt fails at EXEC203 before executing the SDK. Its rendered
BindPaths reset followed populated BindReadOnlyPaths. systemd262 documents
that either empty assignment clears both lists; the read-only executable,
library and notification binds were therefore absent. Resetting before all
entries and checking systemd's parsed read-only bind list fixes the fixture.
The automatic legacy restore passes. No SELinux permission is relaxed.

Trial2 qualifies actual root/sensor readiness and a ten-second idle window.
Trial3 adds the following measured checks:

- Legacy, original Fedora, and original Fedora after root restart each return
  four3-dimensional accelerometer samples, four3-dimensional gyro samples,
  one6-channel front-light sample and one7-channel rear-light sample. All
  reported status3. The existing finite decoder validates framing, timestamps,
  dimensions and finite values; only counts/dimensions/status are retained.
- Manual root restart releases the dependent graph. Both sessions then run
  with fresh PIDs and the original libraries, and the proxy and Adaptive target
  start successfully. HasAccelerometer and HasAmbientLight remain true.
- After requests finish, both process CPU counters remain unchanged during
  ten seconds; swap remains0. ADSP/WPSS remain running with crash counts0;
  CDSP remains offline with crash count0.
- The original cache generation and216 registry-parent files, including214
  leaf registry files, remain. All five saved unit states are restored,
  including active/disabled Adaptive, with no enable preference changes.

These results qualify stationary transport, sample shape, controlled service
restart, limited idle behavior and automatic recovery. They do not establish
sensor accuracy, magnetic heading, native proximity parity, long-duration
stability, DSP crash recovery or suspend/resume. The permanent package still
uses the accepted legacy provider until its versioned namespace/cache/service
transition, dependency closure and signed DNF/reinstall/rollback are qualified.
The legacy protocol remains a documented temporary exception inside Control;
no extra runtime RPM is introduced.

References:

- [systemd262 execution settings](https://github.com/systemd/systemd/blob/v262/man/systemd.exec.xml),
  shared bind-list reset behavior.
- [Qualcomm FastRPC1.0.8 listener](https://github.com/qualcomm/fastrpc/blob/v1.0.8/src/adsp_default_listener.c),
  unchanged original Fedora listener used by these sessions.
- [Trial source](../tests/fastrpc-live-trial.sh) and
  [notification/RAM preflight](FASTRPC-NOTIFY-NAMESPACE.md).
