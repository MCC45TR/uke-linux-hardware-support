# Kernel delivery in Control

7 November 2026: the separate installation-profile package's file requirement
can prevent ordinary DNF upgrades when filelists are not loaded. A device dry
run reproduces the failure. Loading filelists resolves all six systemd upgrade
candidates. The original Fedora `systemd-udev` package still owns `systemd-growfs`.

The candidate retains exact source RPMs Control2.8.1-26/COPR11268045 and boot
integration1.8.102-1/COPR11272089. Control's terminal/client/diagnostic payloads
and the boot, installation-profile and kernel-update runtime become one Control
RPM. The existing optional client SDK remains a separate build dependency.
No previously uninstalled development recovery aliases are added to the runtime.
All source archives remain unchanged and all original build and check sections
run in COPR. No native compilation runs on the tablet.

Compatibility names and feature capabilities satisfy existing kernel and GUI
dependencies. Original helpers retain their paths, while `uke-control` remains
the public settings interface. DNF invokes the retained updater through one
transaction file trigger. Alternating A/B, complete module/boot admission,
rollback defaults and the selected local rollback preference remain unchanged.
The three persistent update configuration files retain `config(noreplace)`.

An explicit migration snapshot preserves enabled, disabled, masked, runtime and
active states of units that legacy package-removal scripts can disable. The
emitted lifecycle fragments are tested with isolated providers. No snapshot
contains calibration, keys, audio data or a partition backup. Kernel selection
and raw slot preservation require separate device admission.

The dependency now names the original Fedora provider `systemd-udev`, avoiding
the filelist bootstrap problem when replacing the old classifier package. A
vendor DNF configuration loads filelists for other retained file dependencies;
administrator configuration keeps its normal precedence. Protected-package,
signature and dependency checks remain enabled.

The candidate needs COPR compilation, actual RPM dependency/script/file review,
upgrade-solver checks and a DNF device transaction before runtime acceptance.
This source change does not establish new driver, encrypted-root, Secure Boot,
TEE key-service, speaker-protection or HDMI timing acceptance.

The first native candidate, COPR11283172, passed the retained runtime and new
lifecycle checks but failed final file packaging: absolute doc/license operands
were interpreted as BUILDROOT paths. The corrected composer stages every retained
boot document under a distinct relative name, including all license texts. The
source-worker fixture rejects absolute operands. No runtime binary was published
by the failed candidate; the corrected attempt retains version2.9.0-1.

COPR11283195 subsequently passes the complete native build and is installed
through DNF. The kernel7331 transaction still refuses before either raw boot
slot changes: both the updater and the private-color helper only parse731.
The Control2.9.0-2 source retains the exact color0.7.0-1/COPR11261358 source RPM
(SHA2565640dbe8a1c3eff89762250c89c81a5f06c5700aff6fa4ae74c234ad2f98d1f1),
all its native checks, licenses and local-only preset admission. It transfers
the nonresident command into Control, with compatibility Provides/Obsoletes.
Factory tables and private calibration remain outside the public payload.

A shared strict parser admits canonical three-part versions from731 onward
and positive bounded revisions with the exact unstable suffix. Existing
same-running-kernel, signed source contract, exact panel, private permissions,
publication and failure-before-staging checks remain. Actual updater and color
fixtures add7331,73334,7402 and8001, including mismatch and running-candidate
refusals. Two native compilers also exercise52 parser cases. Compilation and
device staging remain separate admission gates.

The user explicitly requests a mandatory TUI package. Control2.9.0-2 therefore
requires the exact matching uke-control-tui RPM, rather than merely providing
the terminal binary from the main package. The client SDK remains optional.
The main runtime still owns the installation classifier, updater, client,
diagnostics and private-color provisioning, reducing independent producers
without moving desktop policy or private tables into the kernel.

The required device-local firmware helper also rejects7331. Its0.2-9 source
retains the exact0.2-8/COPR11263798 archive and acquisition/publication controls,
uses the same strict parser, and orders inactive targets by the whole upstream
version and revision. The original exact-source idle-current allowlist stays
unchanged. New synthetic publication tests cover newer versions restarting at1
without touching active consumers. This updates an existing required runtime;
it adds no new producer or resident service. Consolidation into its final
runtime owner still needs separate dependency and license admission.

Control292 first attempt11283433 passes the actual73-case updater guard in both
compilers, then stops when an existing isolated firmware mutant omits the newly
included release header. Its copy list gains that header, retaining every
original mutant and assertion. No binary RPM is published; the corrected
attempt retains2.9.0-2. The source worker compiles the isolated retained consumer
before starting the full native build to catch this missing-input class early.
