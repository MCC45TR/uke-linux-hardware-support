# Original Fedora FastRPC production transition in preparation

Source work only,12 November2026. No production recipe, native build or RPM
installation is claimed by this document. Control2.17.0-6 remains the accepted
provider. Bounded live trial72f8f069 qualifies prerequisites, not the complete
transition.

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

Remaining implementation: production root/sensor units and static RootDirectory,
mount/cache ordering and namespace flags, refresh trigger/state transitions,
actual Control6 recipe inversion and unchanged-source closure, removal of the
legacy protocol ELF from runtime, native tests, signed RPM audit, DNF upgrade,
fresh/reinstall/archived6 rollback and measured runtime/physical gates. Preserve
all existing Control features, output names, enable/mask preferences, cache
recovery and accepted kernel/recovery. Do not retire the working provider on
the strength of unbuilt source.
