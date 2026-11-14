# UKE-CONTROL-MOTION-20261113: lease a proved IIO frame without polling

Date: 13 November 2026. Environment: Uke, Fedora46 AArch64,
Control2.17.0-15 and Linux7.3.3-24-unstable. Source preparation only; no native
compilation, buffer configuration or sensor sampling has run on the tablet.

The separate keyboard's numeric client passes42/42 tests on three Fedora ARM
targets at sourcedd22b9c8/COPR11318576. The signed49 candidate remains outside
normal updates and is not installed. Its45/40 policy cannot use SensorProxy's
coarse tilt strings. Existing Control diagnostics take finite snapshots; they
are not a leased production stream. Motion1 belongs inside Control, not another
runtime package. The kernel/DT owner retains the cumulative queue and frame,
timestamp and hardware-lifetime admission.

## Sources and bounded findings

- Uke native SSC source and shared handoff retain the pinned SNS proto revision
  `32955196`. Gravity has six scalar values (gravity and linear acceleration),
  but its display-relative transform is not admitted. RIGID_BODY19 is an integer
  array of physical-body associations; DISPLAY0 does not establish axes. Attr20
  is reserved. No PLACEMENT decoder or copied accelerometer matrix is justified.
  No protocol code or fitted calibration is copied into userspace.
- The pinned donor survey and hardware acquisition inventory identify libssc
  `54dd13e36d65c21263cf5b0f4a70c230a77b26bc`, Hexagon RPC
  `da7a3742ab8d5273a281b70ee11f16fb2cb6a690` and original SensorProxy as relevant
  comparisons. Their protocol/discovery work does not supply the required
  standard numeric D-Bus API. The new consumer uses kernel IIO instead of
  duplicating those hardware protocols or adding a userspace fork.
- Original [Linuxv6.17 IIO core](https://github.com/torvalds/linux/blob/v6.17/drivers/iio/industrialio-core.c),
  `iio_chrdev_open/release` (GPL-2.0-only, Jonathan Cameron and contributors),
  rejects another character-device opener with EBUSY. Closing releases that
  ownership without disabling buffer capture. Explicit scoped restore and
  crash cleanup are therefore necessary. This older stable ABI reference is
  not proof that the currently fitted source has a valid frame or sample age.
- The [standard buffer ABI](https://docs.kernel.org/iio/iio_devbuf.html) describes
  channel indices, storage types, alignment, watermark and enable-last ordering.
  The consumer must read those declarations rather than assume a packed
  three-number vector. The fitted six-channel scalar layout and timestamp need
  separate actual-source and native qualification.
- Original [libsystemd event watchdog](https://www.freedesktop.org/software/systemd/man/latest/sd_event_set_watchdog.html)
  runs only while its event loop makes progress. Together with a systemd
  watchdog it can bound a frozen broker that still owns its bus name. Its
  periodic health notification is not hardware polling; no lease must mean no
  sensor work or persistent health timer. Provider unit/policy admission is
  not established by reading this API.

## Implementation boundary

The MIT C++ model decodes bounded scalar IIO layouts, requires an
explicit orthonormal mount, preserves the signed face-up/face-down distinction,
and validates stream epoch, ordering and actual sample age. It updates the
latest snapshot on each valid sample but publishes only known/band changes.
Its new test source is uncompiled. Missing frame and invalid data remain unknown.

New source adds asynchronous non-interactive Polkit requests, per-unique-name
leases, targeted signals, cancellation, caller loss and suspend/session teardown.
An event-driven IIO consumer requires its own gravity mount, scalar scan metadata
and boottime timestamps. Enable-last configuration has a root-only restoration
journal; recovery requires the same sysfs inode/device and an exclusive matching
character descriptor. A foreign setting prevents all restoration writes. The
service requires a systemd watchdog and exits after its last lease/pending claim;
it has no sensor poll timer. The separate scalar, transaction and actual-broker
private-bus tests are uncompiled; native validation is the next gate.

Production admission is deliberately closed in source: neither installed24 nor
candidate25 proves the gravity frame and acquisition-time contract. A subsequent
kernel capability is not yet defined. Do not open a device, select another
sensor or infer admission from a version, sensor name or arrival timestamp.
Narrow SELinux integration, actual IIO/journal fixtures, watchdog supervision and
the complete16 recipe/delivery still remain. No raw access is granted to
ordinary users. Use the existing Control runtime and keep
its paired TUI and Core24 SELinux capability unchanged. Derive a unique16 from
accepted15 inputs; do not rerun the published fixed15 factory. Signed native15
SRPM SHA256 is `d6be728b7fb2bb52497dbd91470249ff276e857d2de60db2d5e8659ca52bb71a`;
both signatures use the trusted project fingerprint
`2204a661ea630a5303500085538797e72a3e4eca`. Its spec is byte-identical to the
published `5a0b9d5236a9d071095e77ce0dbe6cb29b80a7f8968b972e3295327637723efc`.
The earlier source-worker SRPM has digests but no signature and is not this
signed parent. One required source reference is retained; the duplicate was
removed after its exact hash was checked. The active
Control worktree and all previous payload/overlays/settings remain untouched.

## Remaining validation

Qualify scalar decoding, byte alignment, timestamps, owner loss, malformed
data, partial setup, crash cleanup, foreign configuration and authentication
through COPR. Verify the complete inherited source/payload/dependency and
service-state closure before any DNF delivery. The current gravity mount,
hardware-time conversion and physical45/40 behavior are not established. A
streaming-cache callback fix does not by itself prove those properties.

First focused COPR11318696 uses immutable sourcea6b4e604 and manifest8b79e705.
All15 asset hashes match. GCC16 compiled the service and scalar test with
`-Wall -Wextra -Werror`; compilation of the transaction test then failed on
misleading indentation. No tests ran and no receipt/runtime RPM was produced.
Braces correct the test without reducing warning flags or its assertions.
Additional source validates journal attribute values and tests the actual
non-interactive Polkit encoding, bounded asynchronous failures and cancellation.
Those updates require a new native run; the first job is not qualification.

Focused11318702/source6eefac90/manifest435886f5 passes all four fixture groups
in the Fedora45 x86 source worker with GCC16 and unchanged warning/assertion
flags. Its actual worker script matches the request. The result directory now
contains a spec and sources, as required by the official custom-source API;
the previous keyboard diagnostic's SRPM-only result was not accepted output.
An empty target list selects the project's default chroot: the56-second
Rawhide ARM job built only a noarch receipt, not the component's ARM objects.
The receipt has two trusted signatures, SHA256c676a7d9; it installs one text
file and has no scriptlets. Temporary producer3282524 is auto_rebuild=false
and remains necessary while this qualification work is active.

Read-only tablet scan metadata then corrected the linear-acceleration names
to `in_accel_linear_[xyz]`. All seven scan indices/types are observed, not
assumed; current scale is0.000001 and the inactive buffer uses realtime.
No sysfs configuration or character device was opened. A separate journal
file layer now supports actual private-file tests of atomic no-replace,
abandoned precommit cleanup, bounded data and mode/owner/hardlink/symlink
refusal. Those changes and the exact scan names await the next native run.

Focused11318757/source5322bce5/manifestc1e232f5 passes all five groups,
including actual private journal files. The worker script and19 inputs match;
the57-second ARM job is again only a noarch receipt. Component ARM compilation,
device IIO ownership and the provider's actual systemd supervision remain open.

SELinux integration is now prepared against original Fedora policy
`5df11316337c290e108c8e8df637c31e87494c08` (GPL-2.0):
`policy/modules/contrib/iiosensorproxy.te/fc` and the standard systemd/init
interfaces. The exact TE SHA256 is
`56d138efdd37b2e29ecae5776688c356658e37ba175ab58b31b4bed0cafc6d28`.
That original domain already reads IIO and manages sysfs. The new helper reuses
it with an explicit mandatory unit context, empty capability set and no new
raw-device/user grant or shared-sysfs relabel. A small separate local module
adds only its private runtime journal, session-status/watch access, watchdog
notification and Motion1 D-Bus communication. Original policy files, SensorProxy
and Core24 remain unchanged. A disabled/missing original domain must fail
activation; do not silently enable a user-disabled module or fall back to init_t.

The remaining userspace delta is the numeric, authenticated, demand-leased API
that original SensorProxy does not expose. Kernel/DT still owns hardware frame,
timestamp and resource admission. If an original upstream service gains the
required numeric semantics and lifecycle, port the client and retire this
internal helper through a parity-checked DNF transition. The new local policy
and a real sd-event heartbeat/frozen-loop fixture are not yet native-qualified;
the latter simulates supervisor termination and cannot establish systemd unit
or tablet acceptance. An authorization-setup exception also gains a regression
test to ensure one cancellation reply rather than a second method error.

Focused11318767/source502c7087/manifestd03b4118 verifies all22 inputs and
compiles the service plus the new local policy using Fedora's original policy
development package. Its new watchdog fixture then fails to compile because
`<cstdint>` was missing; no fixture or Clang/sanitizer run occurred in this
attempt. Add the explicit standard headers and rerun the same gates. This is
not a runtime policy load or device result.

## 14 November: resume from signed Control16 and optional policy fixture

The tablet now runs Control/TUI16 and kernel26. The integration parent changes
to signed Control16/11321859: SRPMf1ef6c120692a8146cc288df70640c05b2be62321b60061f55103732566fada2,
recipee370e5b0705f7d4f6cfbff34ecc915c3eeb3e6fd582405b0d2f9e238d452b3a9,
64 non-spec inputs. All63 inherited Control15 inputs remain identical; preserve
the additional closed-cover health source and paired TUI. Prepare17, without
editing the other workstream's boot/kernel sources or installing anything.

Native5/11318772 previously passed all six GCC and Clang sanitizer groups,
including the private bus, authorization, journal and simulated watchdog tests.
It did not link the new module against the complete original policy store.
The new policy makes sensor-domain grants optional while keeping its own exec
and private-runtime types defined. The mandatory unit context still prevents
fallback when the user disables original iiosensorproxy. A new private-root
fixture links the unchanged Fedora45.24 original policy with that domain enabled
and disabled, preserves all priority100 HLL bytes and disabled preferences, and
checks removal of only the new types. This is prepared source, not native or
systemd/device acceptance. Own-module-disabled upgrade and foreign ownership
lifecycle checks still need package integration; this fixture does not claim them.
Production motion admission remains false; kernel26's diagnostic counter does
not establish the gravity mounting frame or measured acquisition clock mapping.

Policy6/11327628 at92c9b9af passes the new private-store fixture. Actual worker,
23 source assets and original Fedora RPM0aea7d66 match. Original priority100 HLL
bytes, disabled bind/iiosensorproxy preferences and optional type removal all
pass real semodule/checkpolicy operations. The source-worker report is an
unsigned SRPM output with valid digests; signed final receipt admission is
separate. No C++ group was rerun in this policy-only job (retain native5), and
no ARM component, unit, ownership lifecycle, frame/time or device claim follows.

The remaining package-policy lifecycle source is now prepared. The existing
standard PP build converts to marked CIL through Fedora's original HLL compiler.
A transaction-only helper operates in the selected policy stores, preserves a
user-disabled owned module during reinstall, refuses other priorities/unmarked
ownership, and removes only its managed module. No original module is enabled;
original sensor-domain disable remains authoritative. Private fixtures call the
actual helper and compare complete module/policy hashes after foreign refusal.
Disabled-owned, competing-priority and same-priority foreign scenarios are new
unqualified cases. No RPM scriptlet integration, runtime17 or physical claim yet.

Policy7/11327657 at050bd431 passes all new actual-helper lifecycle cases plus
the retained original-store checks.24 source assets, worker and original Fedora
RPM hashes match. Both conflicting-priority and unmarked400 install/remove
attempts refuse without changing any module or policy bytes. User-disabled own
and distribution modules survive reinstall. Signed text receipt2b34292ad6914383
has two trusted2204 signatures, valid digests, two text payloads, no scriptlets
and only rpmlib requirements; its ARM worker packages a noarch receipt. Production
C++ remains the prior native5 baseline. Full17 scriptlets/rollback/unit/ARM build
and kernel frame/acquisition-time admission are still unqualified; no install.

## Control17 source assembly and package lifecycle preparation

The new renderer derives only from signed Control16 recipee370e5b0. Its eleven
additive blocks remove to reproduce the complete parent recipe byte for byte;
three Release fields and twelve versioned package/dependency/provision tuples
advance together. The first local guard expected six tuples and refused the
parent; inspection established twelve metadata fields, all now retained. The
rendered recipe is d1dffafd5be1e01c986d94f516f57a2ca7e27affb8347d3ef3239824fe09cd59.
The inherited SSC state/refresh/postun data fixtures remain unchanged and pass.

The transaction-only lifecycle records one owned unit privately, stops it before
outgoing files disappear, and restores only an active, still-owned, unmasked
incoming provider after the old package hooks finish. It never enables, unmasks
or presets units. Erase/downgrade embeds the same tested policy ownership guard
so cleanup does not depend on the removed helper file.21 preference/activity
combinations plus initial install, downgrade/erase, new masks and malformed/link/
foreign/actor-failure cases pass private recording actors, including the actual
subshell/error-propagation wrapper. These are data/shell fixtures, not a real
systemd, RPM transaction, IIO or device result.

The prepared source factory checks all64 signed16 input names/sizes/digests,
verifies its two signatures with the pinned public key, and appends only the
Motion overlay and build-only original Fedora policy fixture. The archived
parent16 URL was streamed and matched f1ef6c12 without retaining another copy.
The31-source-asset packet and exactly three existing package names are prepared;
COPR source parsing, complete ARM builds and actual service/rollback acceptance
remain open. No original provider, device state or production admission changed.

Integration8/11327821 verifies all31 assets, all64 signed16 inputs, both trusted
signatures and the exact d1dffafd recipe inverse. Source assembly then refuses a
missing ssc-unit-state.sh: the three retained SSC fixture dependencies were not
in the downloaded packet. No candidate17 SRPM or native build was produced.
Add those three unchanged fixture sources to the sealed34-asset packet; preserve
all tests and the rendered recipe, policy, unit and production C++ bytes.

Integration9/11327824 at9c31cc3e passes the complete34-asset source factory:
all64 signed parent inputs, both signatures, full inverse, inherited SSC and
new lifecycle actors, exactly three package names and67 SRPM header files.
The Motion archive is20a40ae01ec2680b55df42ed51d6a4721ab4a76b53e8ba70448c1317ed1a0ba2.
Signed text receipt6880f9ee has both trusted2204 signatures and explicitly states
that no native component/unit qualification occurred. Default Control producer
3266603 remains script53087127/auto=false; a fresh complete53-build inventory
contained no17 or live Control build.

Full17 attempt11327847 then finishes its worker but fails import with "no spec
file available": this custom-source endpoint expects flat spec/sources rather
than an SRPM-only result. Its source_package remains null; no runtime17 NVR or
native package was imported. The corrected adapter extracts the already-verified
candidate into67 flat files and checks all64 parent inputs and spec again. Frozen
9c31cc3e, reciped1dffafd and overlay20a40ae0 remain identical; no production code,
acceptance gate, default producer or device state changes for this correction.

Corrected full build 11327855 imports the expected Control 2.17.0-17 source and
starts the Rawhide ARM worker. Published source a139d67ab8a7b5ca174bfdf872025f3726140d2602be5cf47764eae64bfa7d53
has 67 header files: all 64 inherited inputs retain names, sizes and hashes;
recipe d1dffafd, Motion archive 20a40ae0 and the original build-only policy fixture
match the source qualification. The default producer is independently unchanged
after submission (script 53087127, auto_rebuild=false). This establishes source
delivery only; native checks, signed runtime packages, DNF and real unit/IIO
acceptance remain separate. No local compilation, installation or boot change.

The ARM job 11327855 fails after 536 seconds at the new payload check. Both six-
group GCC and Clang sanitizer suites and the original-policy ownership fixture
pass first. The check incorrectly compares the unstripped build ELF with the
installed ELF after Fedora's brp-strip, brp-strip-comment-note and brp-strip-lto;
their section-table offsets differ. No runtime RPM is published and the inherited
checks after this new check have not run. Candidate 18 retains the exact same
production C++, unit, policy and lifecycle hooks, but replays those same three
Fedora tools on a private copy before requiring byte-for-byte executable equality.
It still derives from signed 16 and preserves all 64 inputs and the complete recipe
inverse. A new release avoids reusing the published 17 source identity. Native
qualification of this correction and real unit/DNF acceptance remain open.

Fresh device ownership queries correct one statement in the frozen Motion1
contract: the installed SensorProxy runtime is uke-iio-sensor-proxy 3.9-24,
with uke-libssc 0.4.4-26, rather than the original Fedora SensorProxy RPM.
The service and executable belong to that retained adaptation. The reused
SELinux domain and base policy are from the original Fedora policy. Motion does
not remove, replace or restart the existing provider; its retirement still needs
kernel/DT support and measured parity. Correct the packaged contract before
normal publication. The frozen build 11327890 remains source 39f862f2; this
ownership correction does not change its production C++ or admit sensor data.

Build 11327890 at 39f862f2 succeeds in 1,408 seconds. Both six-group compiler
suites, the original Fedora policy-store fixture, exact post-BRP payload and
private lifecycle checks pass, followed by all inherited Control checks. Signed
main RPM 4921d868, TUI 22d0a188, devel a450bce8 and source 91145287 each have both
trusted 2204 signatures. All 67 signed source header files equal the reviewed
source-worker inputs. The 38 inherited ELF programs/libraries retain loadable
code, ABI and library requirements; 251 data files, eight links and equivalent
RGB policy are unchanged. Eight declared Motion paths are added. All inherited
scriptlet statements and triggers retain their order after the six declared
additions; the complete source recipe inverse also protects heredoc whitespace.

The RPM audit initially rejects three additional libsystemd symbol requirements.
ELF inspection attributes them to sd_uid_get_display (213), sd_bus_match_signal
(237) and sd_event_add_time_relative (247) in the new Motion binary. Existing
systemd >= 256 and installed systemd-libs 262.2 satisfy them; no dependency is
removed and no additional runtime package is needed. The other explicit addition
is the restorecon executable. A read-only DNF preview resolves only Control/TUI
16 to 18 (two packages, approximately 2 MiB download and 167 KiB additional
installed storage); assumeno aborts without applying the transaction.

Preparation for 19 corrects the packaged provider ownership/status text and
advances release metadata without changing production C++, unit, policy or
lifecycle bytes. Its recipe equals 18 after release normalization and still
inverts to the complete signed16 parent. No 19 build is submitted. The actual
18 unit, SELinux execution, DNF upgrade/rollback and frozen-loop watchdog tests
remain open, as do numeric frame/time admission and physical keyboard acceptance.
Normal producers and installed Control16/Uke39 remain unchanged.

## Actual Control18 DNF transition and execution refusal

The qualification tablet completes a signed DNF Control/TUI16 to18 transition
and a supported downgrade back to16, with only those two packages changing.
Installed headers match the audited artifacts; original preferences, input-method
selection, PowerDevil PID/restarts, existing unit states, policy HLL/module
preferences, warm noswap cache and accepted26/B policy are retained. The normal
kernel trigger reports no slot update. The new module is removed by the packaged
postun helper on downgrade. No sensor capture or keyboard update is performed.

Actual bus activation fails at203/EXEC. Its AVC is init_t denied execute on the
correctly labeled uke_motion_exec_t, before the broker starts; modes and parent
search permissions are correct. The [original Fedora45.24 sensor policy](https://github.com/fedora-selinux/selinux-policy/blob/6610cea35b0821902deaf753fbda3863117b5e15/policy/modules/contrib/iiosensorproxy.te)
binds its executable with init_daemon_domain. The same revision's init.if shows
that init_nnp_daemon_domain grants only process2 NNP/nosuid transitions, not the
new executable entry.19 adds the standard daemon binding for that extra entry,
inside the existing optional block, without changing the original provider or
relaxing global enforcement. A linked-policy permission query and a separately
compiled missing-entry mutant are prepared; native acceptance is pending.

A private acceptance fixture initially stopped before activation because the
runtime WatchdogUSec property was infinity. Original [systemdv262 service code](https://github.com/systemd/systemd/blob/v262/src/core/service.c)
initializes watchdog_original_usec to infinity and snapshots the configured
interval at start; its service.h getter exposes that runtime value. The fixture
now checks the configured2s before start and the actual runtime2s afterward.
ServiceWatchdogs stays enabled; no systemd or watchdog setting is modified.

Source review also finds an activation-grace gap: disappearance of an unrelated
system-bus peer calls release/unused without owning any pending request or lease.
19 restricts immediate idle exit to actual owners. A private-bus fixture uses the
production exit-when-unused mode and verifies unrelated peer loss, a subsequent
claim and last-owner release. An exact SHA-matched pre19 broker is the negative
control. This case is not yet native-qualified and was not reached by the failed
18 runtime activation. The tablet is restored to16; numeric admission remainsfalse.

Native source check10 / 11328124 stops while compiling the policy, before the
six-group tests or a receipt SRPM are produced. An apostrophe in the new comment
inside the M4-quoted optional_policy body terminates its quote; checkmodule
reports an unrecognized trailing quote. Rewrite that comment without the quote,
retaining exactly the same grant, and refresh all37 source seals. No device
policy edit or runtime package change is used to bypass the failure.

Native11 / 11328134 compiles the corrected policy and service, then passes the
sample, ownership and journal cases. The new broker fixture fails its last-owner
check because it expects SD_EVENT_EXITING immediately after sd_event_exit. Original
libsystemd v262 sd-event.c sets exit_requested/exit_code first; dispatch changes
the loop state later. Use sd_event_get_exit_code to assert both absence of an
exit request after unrelated loss and a zero exit request after owned release.
The production broker and both required behaviors remain unchanged. No receipt
SRPM, Clang suite or linked-store acceptance is claimed for11.
