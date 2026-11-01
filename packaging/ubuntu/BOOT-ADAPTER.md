# Ubuntu host boot adapter preparation

Recorded 1 November 2026. This is a bounded host-integration overlay for the
exact release1006 boot recipe. No source from that new set has been executed.
The source-specific native approval is pending; the boot action still exits78.

`debian/boot-adapter/stage.sh INPUT NEW_OUTPUT` checks the immutable emitted
boot lock and every source asset before copying it. It applies a three-file
host integration patch, adds the project-owned Bash provenance module and
records original and ported file hashes in `ubuntu-adapter.json`. It does not
rewrite the original lock to hide changes or claim the ported files are still
byte-identical to Fedora. It executes no shared source script or native helper.

The overlay accepts the bounded Debian `u1` release suffix, uses dpkg package
ownership/copyright inventory instead of RPM and changes a descriptive unit
label to Linux. It explicitly preserves userspace bytes through dracut with
`--nostrip` so provenance checks can compare package payloads. Actual root
admission, switch-root C++, userdata handling and kernel configuration remain
byte-identical to the pinned shared source. Module signing and both Image
passes are unchanged. No security setting is selected or changed.

## dpkg provenance

The host-only module records package, version, ARM64/all architecture, source
package/version and copyright documents with common license references.
Payload paths retain content hashes and one explicit origin kind: package
ownership, relocated package content match, registered shared project source
or non-executable generated metadata. The last category is never represented
as package-owned. Unknown ELF, interpreter scripts and executable files fail.

Usrmerge and alternatives are resolved within the selected host root before
ownership lookup. Diversion explanation lines are not treated as package names.
Owned payload bytes must match their selected input; foreign architectures,
ambiguous owners and missing copyright files fail. Authentication of the host
package set remains the caller's signed-APT responsibility, not a claim made
by dpkg's local metadata alone.

The metadata fixture uses a real isolated dpkg status/info database. It covers
Multi-Arch and usrmerge ownership, relocated dracut hooks, shared assets,
generated metadata and license records. Negative cases refuse changed owned
bytes, unknown non-executable ELF, unknown interpreter scripts and foreign
architecture. No fixture payload is executed. Static staging on the real1006
source passes; source CI also rejects a changed root-guard asset before output.
Native dracut operation remains untested until approval.

## Userspace and security boundaries

[Native userspace qualification](https://github.com/MCC45TR/uke-linux-hardware-support/actions/runs/41319778138)
passed signed official Tenacious and PPA installation, no Python packages or
script payloads, policy-file presence and the original preset hash. Its file
policy omits only identified optional Apport/GDB diagnostics; it records base
exclusions and leaves runtime libraries intact. No root image is exported.

That test installed optional SELinux data to explore compatibility with the
unchanged shared kernel. It does not select the final image's MAC profile or
assert that Ubuntu stock uses SELinux. Ubuntu's default is
[AppArmor](https://documentation.ubuntu.com/security/security-features/privilege-restriction/);
the current shared configuration lacks AppArmor support. SELinux policy
packages alone do not prove an enforcing Ubuntu handoff. Any final security
configuration change requires approval; neither option is silently forced.

Remaining runtime qualification includes the approved native boot helpers,
dracut base, same-job Image/DT/module trust, selected root layout, security
profile decision and actual root handoff. Do not promote the candidate-only
empty-initramfs Image or emit boot artifacts before those checks pass.
