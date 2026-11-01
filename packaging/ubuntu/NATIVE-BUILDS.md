# Native Tenacious ARM64 server evidence

Recorded 1 November 2026. The [initial native run](https://github.com/MCC45TR/uke-linux-hardware-support/actions/runs/41308345325)
uses packaging commit 4642c7d (see the complete commit ID in
`native-build-results.json`) and the unchanged reviewed source pins from
6fb34ec406987866ceee5d38a3eafb393fc4b27b. The JSON is a recorded snapshot;
the live workflow may have advanced.

Each package uses a fresh native `ubuntu-24.04-arm` server and a minimal
Tenacious root bootstrapped from Ubuntu's signature-verified ARM64 ports
archive. Compilation runs as an unprivileged builder inside a disposable
container after architecture and release assertions. Real Build-Depends are
installed; `dpkg-buildpackage` checks them without `-d`. SensorProxy first
builds and installs the exact shared patched libssc in its own build root.
No external ABI dependency is replaced with a fake provider.

Artifacts retain source checksums, `.dsc`, original and Debian source archives,
`.deb`, `.changes`, `.buildinfo`, dependency versions, archive InRelease, OS
identity and complete build logs. Debug symbol packages are not publication
deliverables. Build directories and transient module private keys are excluded.
GitHub retains these artifacts for seven days; copy qualified evidence before
expiry. No user upload private key is present on the server.

The first eight downloaded source-route artifacts produced ten binary packages.
Their artifact checksums passed. Direct payload inspection verified ARM64 DEB
architecture, AArch64 ELF64 executables, and absence of Python files,
interpreter scripts and direct Python runtime dependencies. Those routes were
speaker orientation, installation profile, DisplayPort audio, Bluetooth,
battery observer (three binaries), proximity capture, speaker controls and
front color. This does not prove transitive runtime closure or hardware
behavior. The later driver runs this same inspection before accepting outputs.

The first bootstrap trial stopped because Git configuration preceded its
installation. The corrected order passed. A separate firmware lifecycle trial
passed install, reinstall with a modified conffile, remove and purge, then
stopped at recipe export because dpkg-source creates `.pc` extraction state.
The corrected exporter excludes that state and preserves all upstream files;
the follow-up [recipe test](https://github.com/MCC45TR/uke-linux-hardware-support/actions/runs/41308699685)
passed, including source construction with official git-build-recipe
0.3.7build1. See [the Launchpad handoff](LAUNCHPAD-RECIPES.md).

Build success does not admit boot, service, firmware or stable use. Runtime
closure, service lifecycle, boot cohort qualification, remaining ports and
device acceptance remain separate requirements. Stable and Ubuntu system
images remain gated.
