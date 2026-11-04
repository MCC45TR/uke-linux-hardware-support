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
