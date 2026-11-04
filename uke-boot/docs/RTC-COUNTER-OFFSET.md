# UTC from the Uke read-only PMIC counter

5 November 2026. Running7.3.1-11003-unstable binds PMK8550 rtc0 but exposes the
raw counter calendar in1970. The requested UEFI-variable provider is absent.
This candidate adds a Linux UTC anchor to the existing installation-profile
clock save/restore path; it does not enable PMIC writes or change the kernel.
Native compilation and execution of fixtures are confined to COPR.

## Source basis and scope

The exact Uke counter-only RTC admission is already qualified in kernel7300
and retained by11003. The PMK node, raw class driver and absence of direct-write,
NVMEM and UEFI declarations are checked before sampling. The kernel's sysfs
counter is read through an fd tied to the measured class. No RTC character
device, firmware partition, OEM persist file or EEPROM is opened by this path.

[Qualcomm time-services5e32297](https://github.com/qualcomm/time-services/blob/5e32297bf4d83d37f893a3eb1d6e94f40754df82/time_daemon.c)
separates an advancing RTC counter from a persisted generic UTC offset. Its
BSD-3-Clause shared Qualcomm implementation is a contract comparison, not an
exact-Uke userspace build or a qualified QMI/modem port. The observed stock
eight-byte offset plus the Uke counter reconstructs synchronized UTC within
120seconds in a read-only check. No private offset is exported or copied.
Linux7.3.1 rtc-pm8xxx and the existing Uke secure-offset comparison retain
their own read-only/write-admission rules. No foreign SDAM location or PMIC
permission is introduced. This MIT implementation copies no vendor code.

## Private Linux record

The existing root-owned installation-state directory holds one32-byte file,
`rtc-counter-offset`. It contains a version tag, counter/UTC reference pair,
CRC32 and reserved zero bytes. CRC detects accidental corruption; root ownership,
private0600 mode, one link and the existing exclusive installation-state lock
are the trust boundary. Raw values never appear in logs or Git records.

Only kernel-NTP-synchronized UTC may create or replace an anchor. Sampling is
bounded, the counter must advance, before/after clock samples must agree, and
the committed file and current mapping are verified again. An unchanged useful
anchor avoids another write. A fresh trusted sample may repair corrupt data or
a reset anchor; foreign metadata, links and special files are refused. One
small same-directory temporary file is fsynced and atomically renamed, followed
by directory fsync and readback. No partition image or backup is created.

Before Chrony starts, the existing clock restore command can add the elapsed
counter seconds to a valid saved UTC reference, including slow offline boots.
It refuses counter reset/wrap/stall, yields to synchronized time and never
moves the current wall clock backwards. It verifies the applied clock.
Uninitialized or refused optional counter state retains the existing saved-floor
path and reports deferral. NTP synchronization is never fabricated or cleared.
The original secure RTC commands and kernel updater sources remain unchanged.

The existing Chrony drop-in additionally wants a static counter-sync timer.
Its raw-counter-only service runs the existing clock-save command after boot
and every five minutes. It has no capabilities, no character-device access and
write access only to the existing installation-state directory. It cannot set
the system clock. Profile reconciliation also provisions the anchor when trusted
UTC is available. `clock-counter-check` takes a shared read lock and verifies
the advancing counter/private anchor against current UTC without writing either.

## Qualification and delivery

The source producer imports the complete qualified boot-integration1.8.90 SRPM
from COPR11260223, SHA256
`07bcfb2598878b184f4f8df44a610f16fa653d56346fd328e14cd4b0bf9edc5c`.
It patches only install-profile.cpp and the existing Chrony drop-in, adds the
counter header/tests/units and preserves all other native source/tests byte for
byte. Automatic staging, package rollback-enforced defaults and persistent local
preferences remain unchanged. No kernel package or profile selection is added.

Fixtures require GCC/Clang normal and fail-fast ASan/UBSan runs for wire/data
mutation, real private POSIX file operations, elapsed offline time, reset/wrap/
stall, synchronization races, forward restoration and readback.
COPR11261089 completes the full inherited boot/package checks and the four new
compiler/sanitizer runs in282seconds as isolated1.8.92, exact source
5fe366e47e77bf208178d99b8b0dd302d7f06ca8. Final native log gzip SHA256 is
7d69217f4c9578ea0218bc355e3ae43a4868059bb58e10f9823b5a75ea34944c.
The complete signed installation-profile, kernel-update and boot-integration
RPMs verify with trusted fingerprint2204A661EA630A5303500085538797E72A3E4ECA.
Updater and boot scriptlets match installed1.8.90 byte-for-byte. Signed new
units and the Chrony drop-in match the source; rollback default remains
enforced/config-noreplace. These are native/package results, not a tablet test.

The earlier1.8.91 attempt fails only the new fixture's strict indentation
warning after inherited checks pass. Separating the fault-injection conditions
preserves their behavior, assertions and warning flags.1.8.91 remains immutable.
The source-worker's earlier pre-SRPM version guard is also corrected to exclude
unrelated custom-version experiments while rejecting an exact NVR everywhere.

Fresh1.8.93 is prepared for the main unstable repository with the same clock
implementation and fixtures; its own build and signed audit remain required.
Delivery uses DNF; copied executables are not installed or run on the tablet.
Physical provisioning,
read-only comparison, offline boot and long-run drift remain separate gates.
The raw RTC class remains a counter. This does not establish a hardware UTC
calendar, alarm delivery, suspend/wake, full camera/calibration/focus or complete
device acceptance.
