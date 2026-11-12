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

The follow-up composer derives io_uring subjects from the original linked
policy's create grants to io_uring_t, retaining their Boolean conditions and
attribute membership. On the reviewed 45.23 data there are13 source grants;
the compiler expands them to the100 historical io_uring decisions. The six
existing wildcard/system-role grants and the conditional firmware load grant
remain explicit compatibility policy. They are gated by the original grants
and missing permission definitions. No BPF token or memfd capability is enabled.
A synthetic partial-adoption fixture and an already-adopted input must leave
original definitions and decisions in control. The runtime store and trigger
mechanism are deliberately not implemented or accepted by this experiment.

The first source job11316733 ended before native qualification because the
custom worker returned an SRPM instead of unpacked spec/sources.192005c9 corrects
that source contract; job11316745 contains the exact unchanged policy probe.
A follow-up also recognizes sediff's zero-count headers rather than expecting
empty output, and emits stage markers. No result from the earlier observer is
accepted merely because its process ran or produced a binary policy.

Native11317363/release2 compiles both original-policy compositions within81s
(total job) but SETools' complete semantic diff raises MemoryError under the
1.5GiB bound. This is not a confinement mismatch or a pass. The follow-up uses
a COPR-only C++ normalizer over checkpolicy's complete binary-policy CIL output.
It flattens common permission layout,sorts only permission/type-member sets,
retains Boolean branches,label sequence and every other statement,and compares
complete records. A second comparison filters only the documented new class/
permissions to check all existing Fedora45.24 records. Native fixtures and a
real missing-firmware-grant mutant must reject altered records. This is strict
representation equality,not an arbitrary semantic-equivalence solver. The
normalizer is a build tool and is never installed in a runtime package.

Native11317725/release3 passes the C++ fixtures and both compositions but strict
record equality stops;round trips lose19 attributes on both original versions.
Binary CIL decompilation does not retain compiler attribute-expansion flags.
The follow-up preserves each existing attribute with standard expandtypeattribute
false in test-only inputs and first requires a complete unchanged-policy round
trip. No attribute or rule is ignored by the comparison. A bounded record diff
is emitted on refusal. These flags repair the experimental binary round trip;
they are not part of the proposed runtime compatibility module.

Job11317794/release4 succeeds in160s. Actual SRPM4a56684a/spec8fffbe13 and
four executable/data sources plus three binary input hashes match. The signed
text-only reportfd03d6f6 passes both trusted signatures/digests. The unchanged
round trip,full historical normalized records (753e9494),all original Fedora24
old-permission records (a9a49069),seven grants/denials,capabilities/permissive,
negative firmware mutant and partial/full adoption pass. No record is ignored
to obtain parity. These are offline qualifications;original-base detection,
real modular-store/neverallow,DNF lifecycle and device retirement remain open.

The module-store follow-up separates original base CIL definitions from linked
policy rules. A loaded compatibility module must not be mistaken for native
Fedora adoption during reinstall. Rule references use standard optional blocks
so a pre-update snapshot cannot revive a removed or disabled domain. Fresh
record equality remains required with these wrappers.

The real-store fixture uses unprivileged COPR roots with semodule -n -p ROOT,
explicit expand-check=1,and exact existing RGB/SSC policy inputs. It disables
bind after capturing a stale domain snapshot,then tests install/refresh/
reinstall,fork-first rollback and original reupgrade. Original100 HLL source
hashes,the disabled-module setting and normalized records must remain intact.
The RGB binary comes from the verified Control14 recovery asset ea9ad71e and
has hashcabcbfc7;its original MIT source/context/license accompany the test.
SSC policye0eba7d4 is the exact existing3.9-24 input. These are private offline
store operations,no host load or device DNF transaction. Production RPM hooks,
pre-update snapshots and the supported original-provider delivery still need
qualification. Current DNF configuration reports allow_vendor_change=1;an
assumed mandatory vendor override is not the device's current setting.

Actual11317989/release5 succeeds in325s. SRPMe43a7ecd/spece0af8e98 and six code/
data files,three binary pins,RGBpp/SSCcil and accompanying MIT sources match.
Signed report8d83cd11 passes both trusted signatures/digests. Original-base/
linked-input separation and optional stale references retain all earlier gates.
The real private store preserves disabled bind,all original100 HLL bytes and
full records through install/refresh/reinstall,fork-first rollback and original
reupgrade. Composed/refreshed/reinstall/reupgrade hashes are3ba98da2;rollback/
fork-reference184ed1a5. Non-root ownership warnings do not establish root file
ownership parity. No host policy is loaded or report RPM installed. The next
work is existing Control integration,Core dependency closure,RPM ordering,
normal signed DNF and device checks,then recovery/source closure and producer
retirement. Kernel25,boot,stable,charging and speaker limits are unchanged.

The candidate integration now lives under existing Control's SELinux packaging.
The composer gains only a managed-owner comment;its two-input optional-rule
logic is unchanged. A demand-only shell helper captures pre-update policy data,
stages two no-reload links,refreshes from the actual resulting store,and forces
final rebuild before reloading only the live root's selected policy. Alternate
roots always remain no-reload. Original base100 files are read,not overwritten.
Unknown same-name modules or priorities are refused;only Control-marked400
state can be replaced or removed on native base adoption. No service/timer,
new runtime RPM,hardware operation or desktop policy is introduced.

The next private-store test invokes this exact helper for capture/stage/finish,
replaces RPM-owned policy.kern as an update would,checks reinstall/fork-first
rollback/reupgrade,retains disabled bind/original HLL records and rejects a
foreign same-name module without changing it. RPM hooks and actual device
checks remain separate. This source candidate is not installed on the device.
