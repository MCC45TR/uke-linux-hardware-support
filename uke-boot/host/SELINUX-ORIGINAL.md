# Original Fedora SELinux return

12 November 2026: the installed 45.23-1.3 policy defines newer kernel security
classes and permissions missing from signed Fedora 45.24-1. A direct replacement
would leave those checks subject to the base policy's unknown-permission action.
Keep Enforcing and the existing confinement until the replacement is qualified.

The offline probe tests a standard CIL composition: associate three new common
permission sets with the existing system, bpf and io_uring classes, and append
memfd_file. Its 108-rule reference fixture comes from Fedora policy 45.23 commit
8a22f4474f77c7cb32cd6d18e3da75eaef8696de with the existing compatibility patch
ffff3352806f8bc8f95fde8e4705c588525e4e0b. It is a historical test oracle, not a
runtime module. The complete SRPM and original binary policy hashes are checked.
Fedora policy licensing and source attribution remain applicable.

COPR must compare all allow/audit/transition/constraint/label decisions against
the historical fork, compile the same composition with original 45.24, retain
capabilities and permissive types, check seven bounded grants/denials and reject
a missing-firmware-grant mutant. No policy is loaded or installed by this probe.
The original Fedora source for 45.24 is
6610cea35b0821902deaf753fbda3863117b5e15.

A runtime return still needs modular-store/neverallow qualification, derivation
from the current original policy rather than a frozen domain list, adaptation
when Fedora gains any of these definitions, normal DNF trigger ordering,
reinstall, supported rollback and device checks. Required integration belongs
in existing Control; no additional runtime RPM or replacement Fedora base-policy
build is proposed. Do not remove kernel checks or weaken policy to reduce the
package count. The probe reports source/data evidence only.

Primary implementation references:

- [CIL class/common/order statements](https://github.com/SELinuxProject/selinux/blob/3.11/secilc/docs/cil_class_and_permission_statements.md)
- [libsepol class/common resolution](https://github.com/SELinuxProject/selinux/blob/3.11/libsepol/cil/src/cil_resolve_ast.c)
- [RPM trigger execution order](https://rpm.org/docs/latest/man/rpm-scriptlets.7)
- [Fedora SELinux policy](https://github.com/fedora-selinux/selinux-policy)

Hardware/OEM donor code is not used: this change concerns standard SELinux
policy composition, not a device protocol or a kernel security-hook change.
