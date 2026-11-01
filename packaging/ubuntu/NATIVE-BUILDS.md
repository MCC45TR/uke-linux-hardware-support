# Native Tenacious ARM64 server evidence

## Release1006 update

The subsequently approved release1006 composition at
`fcbdae4b757732f0f0e3162c610f19abae290832` passed a clean native Tenacious ARM64
server build on 1 November 2026. The coordinated release is `7.3.1-1006u1`;
the shared ALSA bound remains unchanged. A single producer retained its module
key through both Image passes and removed the private key before export.
Independent offline inspection verified all 823 module signatures against the
certificate embedded in Image, both Android-v4 wrapper readbacks, 90 pinned
early drivers, and the embedded base's 318 file digests and 116 links.

Four inert trial DEBs assembled from those checked artifacts passed exact
dependency and byte-readback checks. That step is artifact packaging, not an
additional source build or Launchpad kernel qualification. The normal source
recipe still builds first-pass candidates; an explicit complete-producer action
now preserves the qualified host adaptations. Root-image composition, runtime
OEM DT, security profile and physical handoff remain unqualified. No public
Ubuntu kernel or image release was produced.

## Earlier release115 package matrix

Recorded 1 November 2026. The [initial native run](https://github.com/MCC45TR/uke-linux-hardware-support/actions/runs/41308345325)
uses packaging commit 4642c7d (see the complete commit ID in
`native-build-results.json`) and the unchanged reviewed source pins from
6fb34ec406987866ceee5d38a3eafb393fc4b27b. The JSON is a recorded snapshot;
the live workflow may have advanced.

Nineteen of twenty prepared source routes now pass native server package
construction: thirteen compile native payloads and six package data only.
The initial matrix passed eighteen; the corrected
[UPower retry](https://github.com/MCC45TR/uke-linux-hardware-support/actions/runs/41309136146)
passed all four binary packages and payload audits. The separate firmware
recipe/lifecycle run also passed. The
[kernel retry](https://github.com/MCC45TR/uke-linux-hardware-support/actions/runs/41309327976)
has terminated with a compilation failure. Its first attempt stopped during the shared preparer's
contracts because the Debian driver had not recorded its successfully applied
patches in `senemos-applied-patches.txt`. The corrected driver appends only
after successful application and compares the complete ledger before invoking
the unchanged preparer. That correction passed preparation; compilation then
failed at `sound/soc/qcom/uke-audio-card.c:411`:
`BUILD_BUG_ON failed: sizeof("uke" UTS_RELEASE) > 16`.
The Debian release suffix `7.3.1-115-uke-ubuntu1` exceeds the shared ALSA driver
identifier bound. No kernel binary or boot success is asserted. On resumption,
use a coordinated shorter Debian kernel release identity within that bound
across the recipe, candidate manifests and inspectors, preserving the shared
driver assertion. Implementation and new native dispatches are paused while
Fedora Rawhide DE release work takes priority. The existing Launchpad source
build remains active and is monitored read-only.

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
