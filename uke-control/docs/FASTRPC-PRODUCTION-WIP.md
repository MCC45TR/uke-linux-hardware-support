# Original Fedora FastRPC production transition in preparation

Source candidate only,12 November2026. The Control2.17.0-10 recipe is prepared;
no native build or RPM installation is claimed. Control2.17.0-6 remains the accepted
provider. Bounded live trial72f8f069 qualifies prerequisites, not the complete
transition.

Candidate7/COPR11311279 completes native compilation and installation staging.
Both the unchanged full cache regression and new migration fixture pass. The
next unit-parser step cannot create /run/systemd as unprivileged mockbuild;
the full parent checks and RPM generation are not reached. Candidate8 returns
to the accepted actual Control6 source baseline, retaining the exact candidate7
production cache, unit and static-root data. It changes only qualification
isolation and the package release.

systemd262 manager_startup creates the runtime prefix before applying --root
to unit lookup paths. A build-only C++ path interposer redirects only
SD_PATH_SYSTEM_RUNTIME for the unchanged system analyzer. Its API probe checks
the private prefix, unchanged forwarding of other path types, and failure when
the scope is missing. The analyzer core library's dynamic import is checked. System scope,
unit settings and the distribution binary remain unchanged; no runtime shim
or altered systemd package is installed. The fixture compiles in COPR only.

Candidate8/COPR11311317 also passes both cache fixtures, then fails at a silent
qualification assertion before the complete parent checks. Its import gate
incorrectly examines the main analyzer ELF. The same original Fedora262.2-1
package on the target has the sd_path_lookup import in libsystemd-core, while
the main executable imports that library. Candidate9 follows the loader's
core-library path and adds named fixture phase diagnostics. Production source
remains byte-identical; full unit-parser/interposer acceptance is still pending.

Candidate9/COPR11311349 passes every new provider fixture and the full existing
native check suite. RPM file processing then rejects the documentation path:
an absolute build-directory path under %doc is treated as an installed payload
path, not a source to copy. Candidate10 explicitly installs the document below
the package doc directory and lists that installed path as %doc. An early native
byte comparison checks its staged location. Production cache, unit and static
root source bytes remain unchanged; RPM/signature/DNF acceptance is pending.

Use the existing Control cache helper with a dedicated16MiB noswap tmpfs at
/run/uke-ssc/fedora, mounted by a standard systemd mount unit. Keep hardware
FastRPC in the native driver and protocol in original Fedora fastrpc. The
existing Control session/notification integration remains the only session
layer; no additional runtime package is needed.

On initial migration, acquire both the new private cache lock and the old
accepted cache's exclusive lock before copying its complete generation. Reject
an active old reader, unsafe directory, incomplete generation or symlink input.
Preserve current RAM registry changes and parent-level metadata rather than
reseeding from factory persist. Refresh config/plugin/SoC through the existing
atomic publication path. A warm new generation owns subsequent changes and
must never be overwritten from stale legacy state. Keep the accepted old RAM
generation during qualification and signed rollback; create no disk backup.
Keep the mount across ordinary cache/provider stop-start cycles, preserving
the existing warm-generation lifetime. Unmount only through shutdown or a
qualified explicit removal/rollback. Automatic StopWhenUnneeded unmounting
would discard that RAM generation and is disabled. The cache unit must verify
the mount's tmpfs/noswap contract before the helper runs.

The new project source is derived from the exact SSC32 GPL-3.0-or-later cache
helper, SHA256 b135389fb30f54ea632db23f6bb1a0a5bd99cb399e19fd74725a791c68bf0afc.
The archived input and donor references remain unchanged. This helper copies
files and integrates session lifecycle; it does not implement FastRPC. The
pending synthetic C++ fixture covers fresh seeding, accepted RAM import, old
reader refusal, modes/incomplete/symlink refusals, unchanged factory input,
parent metadata and independent warm refresh. Run it and all existing cache
tests in COPR, never with the tablet compiler.

Prepared source includes production root/sensor/cache units and a static
RootDirectory buildroot-data helper. The units retain the qualified private
namespace/device boundaries, require the cache mount, verify its tmpfs/noswap
contract and use the original Fedora listener through existing Control helpers.
All four units pass systemd-analyze verify on the current target, without
installation or activation. Static root staging contains empty mount targets
and fixed internal aliases only; it copies no binaries or private inputs.

The candidate recipe preserves all60 actual Control6 source files and adds one
provider archive. Complete recipe reversal and the existing5-unit state and16
refresh-cascade fixtures pass as data-only checks. Runtime installation of the
legacy protocol ELF is removed; the cumulative historical regression sources
and checks remain. The cache binary is rebuilt from the new project source;
the original Fedora libraries and existing Control session/notification helpers
are unchanged. The static root's8 empty targets use private non-executable0600
modes so RPM can read its empty payload during package construction.

Remaining qualification: actual COPR source closure and native tests, mount
removal/downgrade lifecycle, signed RPM audit, DNF upgrade,
fresh/reinstall/archived6 rollback and measured runtime/physical gates. Preserve
fresh-boot original-provider acceptance separately from warm live attachment.
Preserve
all existing Control features, output names, enable/mask preferences, cache
recovery and accepted kernel/recovery. Do not retire the working provider on
the strength of unbuilt source.

## Installed-label correction for candidate11

Candidate10 passes all native tests and signed payload checks, but the installed
service fails226/NAMESPACE before SDK execution. Original systemd262.2 implies
log-socket binds for PrivateDevices with RootDirectory. SELinux denies init_t
creation of the missing dev-log mount target under the installed usr_t tree.
Accepted6 is restored through signed DNF, all seven states and stationary
samples return, and10 is withdrawn from COPR and refreshed metadata.

Candidate11 explicitly disables those unnecessary log-socket binds in both
sessions. The actual usr_t software fixture reproduces the baseline226 and
passes with explicit no; only original sleep runs. A bounded real Fedora
listener trial using the same static label then passes readiness, stationary
accel/gyro/dual-ALS shapes and status3, fresh restart PIDs, zero10s idle CPU
delta and zero swap. Automatic accepted6 recovery removes the owned static
tree, transient overrides and private cache. The first two software-observer
attempts stopped on the immediate executable-path assertion; Type=exec reports
flock before its child exec. The accepted fixture waits for the original sleep
inode before inspection. No policy, label exception or local binary is added.

The cache algorithm, mount, static-tree data and original libraries are unchanged
from10. Candidate11 still requires its own full COPR/source/signature/DNF and
fresh/reinstall/rollback checks. Cold boot, suspend and full sensor accuracy
remain separate acceptance gates. See FASTRPC-STATIC-ROOT.md and the canonical
migration lesson for the failure, recovery and exact original systemd sources.

## Candidate12: retire only an outgoing provider's mount

Candidate11 source/native qualification continues, but device installation is
withheld while the downgrade boundary is corrected. Candidate10 recovery left
its StopWhenUnneeded=no mount active until explicit cleanup. The cache source
correctly preserves its warm generation during normal operation; reusing that
generation after a legacy-provider downgrade/re-upgrade would miss the latest
legacy RAM changes. A supported RPM transition must retire that mount itself.

Candidate12 adds an embedded postun block, guarded by the incoming mount-unit
file's absence and a running system manager. It stops the three old reader/cache
units and their mount in one transaction. Incoming6's existing state snapshot
and refresh restore consumers. Retained-provider upgrades/reinstalls and offline
transactions do nothing; ordinary cache restarts still preserve warm RAM.
Twelve data-only manager/provider/count cases plus failed-stop refusal pass.
The target software fixture uses the actual rendered block with private unit
names and a public noswap tmpfs vector: normal restart and retained postun keep
its inode; removing the unit leaves the mount active until postun, which stops
readers and unmounts. The accepted vector and production service/cache state
remain unchanged. No DSP operation or private input is part of this fixture.

The native synthetic cache test additionally covers re-import after retired
new RAM and a later accepted-provider change. Its native12 execution, expanded
postun test, complete source/RPM/DNF lifecycle and physical gates remain pending.
Production cache C++, unit files, static tree and existing helpers are unchanged
from11; only the package lifecycle, qualification and documentation change.
