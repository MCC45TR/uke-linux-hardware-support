# UKE-CHANNEL-POLICY-20261104: separate upgrades from exact version synchronization

Date: 4 November 2026. Environment: POCO Pad X1, Fedora 46 Rawhide,
AArch64, Linux 7.3.1-1500, installed Control 2.8.1-9, libdnf5 5.4.6.0.
Native compilation and source/RPM checks run in COPR; tablet installation uses DNF.

The earlier progress correction exposed a policy problem: both channel selection
and ordinary updates required every installed project component in the target
repository and forced older destination versions. Stable/test lack eight current
components and their latest kernel is 7.3.1-1010.1, while the tablet's kernel
tuple is explicitly locked to 7.3.1-1500. Copying an entire experimental repository
would also promote unrelated builds. A destination missing an optional installed
application does not justify removing it or preventing selection of that channel.

The new normal policy upgrades available components, retains newer installed
versions and unavailable extras, and reports retained NEVRAs and reasons. Solver
exclusions and final transaction checks prohibit altering retained components,
including indirect dependency replacements. Ordinary updates reject any downgrade.
Only a genuinely newer admitted kernel tuple reaches the guarded updater. Kernel
identity remains reported separately from whether boot work is required.

Exact target synchronization remains explicit in `channel sync`, including its
missing-package, signature, retained-kernel and interrupted-operation guards.
An unchanged normal update returns success without confirmation or a new
RPM/journal/boot operation, after verifying the persisted repository policy.
Versioned operation receipts retain whether the interrupted request was a channel
selection, distribution update or exact synchronization. Recovery replans that
original policy; legacy receipts retain their previous exact-sync semantics.

Signed offline fixtures cover missing extras, newer installed manager/runtime
versions, older or excluded kernels, ordinary channel/update resolution, explicit
sync refusal, incompatible managers, unsatisfied retained dependencies and unsigned
packages. Their RPM database is isolated and populated with database-only fixture
records; no worker-root or tablet payload is installed. Native COPR results and
real device channel round trips remain pending until separately verified.

COPR trial 11254559 compiled the application but failed the new fixture before
policy assertions: its nested rpmbuild inherited worker RPM output conventions,
so the expected architecture-subdirectory path was absent. Fixture builds now
set both `_rpmdir` and `_rpmfilename` explicitly. No binary candidate was
published or installed from that failed trial; native policy validation remains
pending the corrected build.

Builds 11254574 (unstable), 11254580 (test) and 11254581 (stable) passed native
tests and published 2.8.1-10. The first real unstable-to-test transaction then
completed its RPM and channel phases but exited 141. Its JSON output ended at
the 8192-byte stream boundary. The operation receipt was complete, pending state
was cleared and protected boot/version-lock files were unchanged. This is a
process-output failure after a completed transaction, not an unknown RPM result;
the completed transaction must not be replayed to diagnose it.

The coordinator had blocked SIGPIPE together with user cancellation signals.
An internal scriptlet pipe could therefore leave SIGPIPE pending until signal
mask restoration, killing the process before the remaining JSON buffer flushed.
The 2.8.1-11 correction ignores SIGPIPE for the coordinator lifetime, retains
deferred user cancellation, flushes receipts before signal restoration and checks
final stdout completion. A subprocess fixture generates EPIPE under the actual
critical guard and verifies a complete JSON receipt larger than the stream buffer
with a zero exit status. Full device round trips remain pending this correction.

Verification update, 4 November 2026: this supersedes the pending acceptance
status above. Source commit `5c762212f86260daa4ede1311242f8fa73eb5441` produced
Control 2.8.1-11 in [unstable build 11254600](https://copr.fedorainfracloud.org/coprs/build/11254600/),
[test build 11254623](https://copr.fedorainfracloud.org/coprs/build/11254623/) and
[stable build 11254624](https://copr.fedorainfracloud.org/coprs/build/11254624/).
All three native builds and complete retained test suites passed. The five
backend/SDK RPMs were signed by each destination project. DNF installed the
backend/client pair; no copied executable or tablet compilation was used.

The installed tablet completed all six directed transitions, with one additional
test-to-unstable return. Each transition was followed by a successful update and
an empty repeated update. The repeated calls preserved the operation receipt and
repository/selection file digests. A normal confirmation through `sudo uke-control
branch stable` and both branch/brunch aliases passed. The final ordinary sudo
update succeeded with closed stdin and no unnecessary confirmation.

| Channel pair | Both directions | Updates and empty repeats |
|---|---|---|
| stable / test | Passed | Passed |
| stable / unstable | Passed | Passed |
| test / unstable | Passed | Passed |

Every structured transition receipt parsed successfully; observed full receipts
were 9098–10945 bytes, exceeding the previous 8192-byte truncation point. The
acceptance script returned zero. RPM verification of Control and its client was
clean. Final selection is unstable, phase complete, no pending channel and no
recovery action. Linux 7.3.1-1500 remained running. Kernel version-lock, automatic
policy and userdata boot-state file digests were unchanged across every tested
transition. No reboot, slot update or new physical hardware acceptance occurred.
