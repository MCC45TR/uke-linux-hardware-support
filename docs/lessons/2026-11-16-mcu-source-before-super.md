# UKE-MCU-READER-20261116: verify the packaged source before stock parsing

Date: 16 November 2026. Environment: separate development PC, actual successful
managed firmware 0.2-10 sources and POCO Pad X1 kernel 7.3.3-28-unstable.

The public generic MCU image found in the shared OEM vendor source exactly
matches the pre-existing 82,740-byte Uke hash. The existing firmware package17
delivers that file. The managed helper11 checks its protected parent chain,
non-symlink regular descriptor, owner/mode/link count, size and complete hash
before parsing Android super. An absent file permits the unchanged stock
fallback; a present unsafe or altered file refuses rather than falling back.

The complete successful10 recipe/source inverse retains semantic ABI ordering,
exact idle-current source admission, active-consumer refusal, no-replace
publication, native FAT/authentication fixtures and all hardware ownership.
The helper still contains no OEM firmware. Build and runtime requirements bind
it to the existing generic factory-data capability; no new runtime name is
introduced. CDSP/authentication are still acquired read-only from modem. Super
is opened to validate its physical UFS/root relationship, but its contents are
not parsed when the admitted packaged MCU is present.

The original sanitizer contract fixture also exercises the real new reader
against copied public data: exact positive, missing-only fallback, altered hash,
short/oversize/empty file, writable mode, hard links, symlinks, dangling links,
symlinked parents, directory and writable parent refusal. No controller,
EEPROM, stock partition or private calibration is accessed by that fixture.

Source/text checks are separate from native COPR, signed DNF, actual target
verification and full ZIP qualification. Encrypted-root device ancestry needs
its separate mapper-aware validation; this source change does not claim it.
