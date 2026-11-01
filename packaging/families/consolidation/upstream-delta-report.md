# Upstream differences and native interfaces

1 November 2026. This source review is not new hardware acceptance. Source
snapshots must be compared with the pins in inventory.json; main alone does
not describe the live unstable binaries.

The later [Fedora return result](gnu-return-results.md) closes the accessibility,
GStreamer, libwacom and GNU package gates. The table preserves their historical
fork differences; real corrections in the other sources and physical device
acceptance remain separate.

| Source | Retained difference | Removal gate |
|---|---|---|
| uke-at-spi2-core | Optional Python GI override excluded | Signed original Fedora accessibility payload and closure equivalence |
| uke-gstreamer1 | Host documentation helpers excluded; native scanner retained | Original Fedora media/plugin closure and helper provenance |
| uke-libaccounts-glib | Optional Python GI override excluded; upstream destructor/leak correction | Equivalent destructor fix as well as original native API, typelib, file ownership and account smoke checks |
| uke-libwacom | Optional Python updater/viewer excluded | Original library/database, helper origin and tablet metadata checks |
| uke-libstdcxx-runtime | GNU ABI without GDB Python pretty-printers | Exact versioned C++ symbols, security-update and original Fedora payload/closure audit; prefer the distribution runtime when admitted |
| uke-accountsservice | Missing optional TCB directory accepts only ENOENT | Equivalent upstream error semantics and account regression checks |
| uke-pipewire | ELD monitor name used for DP sink/port labels | Equivalent upstream version or configuration, with ELD/hotplug behavior |
| ddcutil | First-line-read error and memory safety correction | Upstream fix identity plus failure and DDC regression fixtures |
| uke-upower | Threshold errors, battery health, wireless classification and late capacity | Equivalent upstream behavior; power_supply presence alone is insufficient |
| uke-libssc | SSC/DSP transport, samples and mount matrices | Equivalent physical IIO presentation, scale, units, dual light and suspend |
| uke-iio-sensor-proxy | SSC, dual light, desktop D-Bus and SELinux integration | Native IIO equivalence; standard SensorProxy interface remains needed |
| uke-mesa | AArch64 JIT, Adreno 732 identity, Turnip display-fd/UBWC | Equivalent Mesa source plus GL/Vulkan/ABI and device regression evidence |
| uke-mesa-libclc22-spirv | Exact LLVM/SPIR-V compiler data | Compatible distribution bitcode/compiler tuple |
| uke-vaapi-v4l2 | Stateful Iris VA-API adapter and runtime format/rate/error/cache checks | Equivalent native userspace backend; V4L2 alone does not supply VA-API |
| uke-fidelityfx | Linux portability and experimental context ABI | Compatible upstream SDK; installation is not frame-generation support |
| material-decoration | Qt/QML metadata corrections for the pinned upstream decoration | Equivalent upstream metadata and KWin ABI |

The five-return proposal needs a correction: the pinned Fedora
`libaccounts-glib-1.25-28.fc46.src.rpm` does not contain the upstream destructor
fix. Its SHA256 is
`d9895473a2e6fe342f8a3975148166aa3bbf74580dee7da4bf57300664efdbcb`.
The unmodified `ag-account.c` still passes `priv->services` instead of
`priv->changes` to `_ag_account_changes_free`; the original spec declares no
patch for this. Uke applies upstream commit
`270a66d5efe1453a2be4bad10894b947fdd2923e` through
`uke-desktop-packaging/packaging/libaccounts-glib/0001-fix-uncommitted-changes-cleanup.patch`.
The extracted original source file hashes to
`b5c79b0f02019d3ddc4728d5c71379e0671a8e31b7fd08952f506d470cdb0f92`.
Returning to that Fedora build would lose a real correction. A newer signed
Fedora source must first demonstrate equivalence; relaxing the helper-payload
policy alone is insufficient.

DT describes documented hardware bindings. Kernel drivers own IRQ, transport,
regulators, input, DRM, media and power-supply mechanisms. D-Bus, session
policy, package updates/rollback, UCM, WirePlumber, UI and host image production
retain their userspace owners. No synthetic DT node replaces a missing SSC
protocol or desktop consumer. Mesa and a VA-API adapter are not kernel drivers.
This follows the upstream [binding design guidance](https://docs.kernel.org/devicetree/bindings/writing-bindings.html):
describe actual hardware and avoid nodes created solely to instantiate drivers.

The donor policy requires Uke OEM, applicable same-SoC/shared-IP and upstream
comparison before a driver change. This packaging change changes no driver or
board value. Existing touch processing/licensing and physical acceptance gaps
prevent fallback retirement. Factory MAC/calibration handoff remains read-only;
no provider with demonstrated equivalent security is introduced. Internal
four-channel playback and separate stereo DisplayPort routes stay distinct.

The GNU policy follow-up also inspected the signed original
`libstdc++-16.2.1-3.fc46.aarch64.rpm`; its SHA256 and key are recorded in
[gnu-console-policy-audit.json](gnu-console-policy-audit.json). It still owns
15 Python helper files, none marked as documentation, so omitting documentation
does not omit those files. The 1 November 2026 policy approval retains the
original Fedora helpers on the Fedora root, including console profiles. The
signed ABI, caller, forward/reverse and initramfs dependency checks are recorded
in the GNU receipt. The Uke GNU producer now refuses another build; complete
source and signed rollback packages remain archived. Original Fedora security
updates no longer require this project to rebuild the GNU runtime. Project
Python and Python in recovery, initramfs or installers remain excluded.
