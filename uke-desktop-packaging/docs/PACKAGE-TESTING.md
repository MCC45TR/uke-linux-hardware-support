# Rawhide AArch64 package acceptance

The current target is a console selection with independent desktop readiness
metadata. The owner forbids cloning, forking or rebuilding KDE applications;
original distribution applications remain required. Their current Python
payloads block a complete KDE selection. Do not use the historical derivative
packages or migration helpers as a new acceptance candidate.

## Current gates

1. Compile the five allowed non-KDE native source families. GNU C++ must retain
   all 6,100 original versioned exports and execute the native C++ smoke fixture.
2. Resolve the current core, data-only desktop metadata, optional theme and
   recovery delivery through DNF with weak dependencies disabled. Store the
   exact nonempty transaction, package NEVRAs, licenses, hashes and signatures.
3. Extract all selected RPMs on the host. Audit Python files, entrypoints,
   interpreter/ELF dependencies and private content. GNU readelf is mandatory;
   missing tools and unreadable ELF fail the scan.
4. Replay the signed transaction in a clean pinned AArch64 userspace under QEMU
   with network disabled. Verify package versions, the GNU C++ capability and
   RPM-owned files. No boot, session or recovery installer is executed.
5. Export the complete installed `usr/` and `etc/` and audit them on the host,
   including inherited base-image files. Package-name checks alone miss optional
   libstdc++ Python debugger helpers. An ordinary signed package upgrade must
   remove those old files; never delete RPM-owned target files manually.
6. Upgrade actual earlier signed console metadata to the new revision and
   repeat verification and full-root gates. Remove the admitted metadata,
   kernel, recovery and theme and check their records and payload are absent.

Never bypass Python conflicts, use `--nodeps`, skip broken dependencies or
promote empty query/extraction output. A changed graph needs a fresh record.

## Signing and historical evidence

The observed COPR fingerprint is
`DAFC3C5A881FB49C7167EE2D6F772E3D487BD13E`; Fedora 46 primary is
`D924B10D3E810DABDD8B56B596E7E91491211FCE`. Verify signatures in isolated
key databases. Exact source hashes identify reviewed official Fedora inputs.

The [current 52-input console record](https://github.com/MCC45TR/uke-fedora-builder/blob/main/reports/CONSOLE-RAWHIDE-2026-10-05.json)
and [exact input lock](https://github.com/MCC45TR/uke-fedora-builder/blob/main/manifests/CONSOLE-RUNTIME-LOCK-2026-10-05.json)
passed the sequence above. Initial core installation explicitly allows the
reviewed Fedora-to-COPR GNU C++ vendor change. A subsequent accidental restart
of an already completed upgrade fixture is excluded; its successful pre-restart
export and separate immutable removal snapshot retain their recorded gates.

The [historical desktop report](https://github.com/MCC45TR/uke-fedora-builder/blob/main/reports/DESKTOP-RAWHIDE-2026-10-05.json)
and [rejected 603-input lock](https://github.com/MCC45TR/uke-fedora-builder/blob/main/manifests/DESKTOP-RUNTIME-REJECTED-603-2026-10-05.json)
retain bounded input, fresh-install, fixture, actual upgrade and removal passes.
Their complete root failed on 15 inherited Python/PYC files. Its KDE derivatives
were subsequently withdrawn by owner policy. These results are historical and
must not be treated as current target admission.

Caches, logs and exported roots are local test artifacts, not tablet images.
Source compilation, signed RPMs and QEMU userspace do not establish a Uke boot,
graphical session, decoration rendering or peripheral support. Upstream host
build tools and pins are documented in [native runtime rules](NATIVE-RUNTIME.md).
