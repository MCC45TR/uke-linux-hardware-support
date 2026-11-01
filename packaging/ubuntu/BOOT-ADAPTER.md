# Ubuntu host boot adapter preparation

Updated 1 November 2026. The approved clean Tenacious ARM64 server build of
exact release1006 succeeded. Its complete two-pass producer, embedded payload
and all 823 module signatures passed independent host inspection. This is
source/build evidence; no root image, hardware boot or public Ubuntu release
is admitted.

`debian/boot-adapter/stage.sh INPUT NEW_OUTPUT` checks the immutable emitted
boot lock and every source asset before copying it. It applies a three-file
host integration patch and the natively tested Tenacious/dracut compatibility
patch, adds the project-owned Bash provenance module and
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
The compatibility layer additionally resolves blkid through the host PATH,
recognizes exact package-content replacements, registers the literal shared
unit drop-in, verifies dracut's generated release metadata, and authenticates
the generated hwdb cache against a root-prepared package/generator receipt.
Generated locked shadow/gshadow tables are checked before omission; unknown
accounts, password hashes and empty passwords remain refused. Passwd/group
and service policy are preserved. All three auditors are host-only.

`debian/build-kernel boot` now exposes that complete producer as an explicit
server trial action after prepare. It requires the external root-owned hwdb
receipt before starting; no package installation calls it. The ordinary
Debian binary recipe still emits the older first-pass candidates. Launchpad's
unprivileged binary builder has no supported receipt preparation step yet;
do not claim an automated kernel source-package route until that is resolved.

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

Native boot helpers, dracut base and same-job Image/module trust now pass.
Selected root layout, security profile and actual root handoff remain open.
The compile-stage DT template is not the runtime OEM DT. Do not promote the
candidate-only empty-initramfs Image or infer hardware support from this build.
