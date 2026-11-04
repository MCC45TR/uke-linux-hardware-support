# UKE-QT-KERNEL-20261104: use driver facts for dock, charging and NFC

Date: 4 November 2026. Environment: Fedora Rawhide AArch64, POCO Pad X1,
Uke Control 2.8.1-24.speech1 and unstable kernel 7.3.1-11003.

## Evidence and implementation

The previous Qt summaries discarded USB link speed, USB Ethernet counters and
external display modes that already existed in kernel sysfs. A bounded fixed
read-only helper now supplies separate accessory, Type-C, Ethernet and display
cards. Link capability, current USB speed, network speed and measured throughput
are distinct. Advertised display modes do not establish active timing or an
observed image.

Unprivileged charge admission returned `control_root_required`. Root admission
instead reports `control_profile_unqualified`; the standard threshold attributes
return ENODATA. The existing charge owner's `settings` command reads firmware
settings without authorizing writes. Qt uses that verified result, independently
of admission, and exposes the existing guarded charge-limit request. A narrow
PolicyKit action authorizes only a fixed read helper for active local sessions.
No threshold, firmware SET, boot, staging or rollback policy write is added.

The fitted FM19511 is an I2C Type 2 tag, bound through its misc-class device;
it is not an external tag/card reader. The helper queries the exact version-1
INFO ioctl, with no memory addressing, raw payload reads, EEPROM writes or IRQ
consumption. Older kernels may return ENOTTY. The exact Uke OEM research is
recorded in `uke-linux-docs/docs/testing/UKE-IR-NTAG-SOURCE-2026-10-20.md` and
`docs/lessons/2026-10-28-UKE-NFC-SUPPLY.md`. The native UAPI source is
`senemos-uke-kernel/src/nfc/fm19511-uapi.h`; FM19510 capacity or other-chip
memory maps must not be assigned to this part.

Explicit NDEF reading uses the distribution's original Qt NFC implementation,
which supports Linux neard and limited PC/SC readers. There is no Qt/KDE fork.
The reader has cancellation, a 30-second deadline, bounded displayed records,
plain-text content and no automatic URI launching, writes or payload logs.
See [Qt NFC](https://doc.qt.io/qt-6/qtnfc-index.html) and
[QNearFieldTarget](https://doc.qt.io/qt-6/qnearfieldtarget.html).

Uke's existing USB role writer performs one attached-partner role swap, without
a persistent forced mode. The kernel UCSI controller handles reconnection.
Qt reports automatic handling only for the verified Uke Type-C profile and
UCSI owner, and retains the guarded Host/Device requests. Automatic inspection
never writes `none`, which means disconnected/disabled in the USB role ABI.

## Validation and remaining work

Source review and fixture coverage are separate from the pending COPR build,
DNF delivery and installed read-only checks. No native compilation occurs on
the tablet. Physical NDEF reading requires a compatible external reader and
an actual tag. Charge cutoff and arbitrary new thresholds remain unqualified;
a successful information query must not become a hardware acceptance claim.
The connected dock's Ethernet and DP output must remain connected during tests.

The installed Fedora Qt 6.12 package contains the neard backend and no PC/SC
backend. Fedora provides original neard 0.19. Release 1.3.0-2 therefore requires
neard and adds an explicit native service-start action; installing Qt alone is
insufficient for external reading. A service start may use the distribution's
systemd/PolicyKit authentication. No reader daemon is automatically enabled at
boot by this application. Root read cancellation is asynchronous even after
pkexec drops the caller's kill permission; the helper itself has an 18-second
process deadline and each fixed charge-owner subcommand is bounded.
