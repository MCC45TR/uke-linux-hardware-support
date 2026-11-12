# Original Fedora FastRPC production transition in preparation

Source candidate only,12 November2026. The Control2.17.0-8 recipe is prepared;
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
the scope is missing. The analyzer's dynamic import is checked. System scope,
unit settings and the distribution binary remain unchanged; no runtime shim
or altered systemd package is installed. The fixture compiles in COPR only.

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
