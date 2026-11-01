# Upstream differences and native interfaces

1 November 2026. This source review is not new hardware acceptance. Source
snapshots must be compared with the pins in inventory.json; main alone does
not describe the live unstable binaries.

| Source | Retained difference | Removal gate |
|---|---|---|
| uke-at-spi2-core | Optional Python GI override excluded | Signed original Fedora accessibility payload and closure equivalence |
| uke-gstreamer1 | Host documentation helpers excluded; native scanner retained | Original Fedora media/plugin closure and helper provenance |
| uke-libaccounts-glib | Optional Python GI override excluded | Original native API, typelib, file ownership and account smoke checks |
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

DT describes documented hardware bindings. Kernel drivers own IRQ, transport,
regulators, input, DRM, media and power-supply mechanisms. D-Bus, session
policy, package updates/rollback, UCM, WirePlumber, UI and host image production
retain their userspace owners. No synthetic DT node replaces a missing SSC
protocol or desktop consumer. Mesa and a VA-API adapter are not kernel drivers.

The donor policy requires Uke OEM, applicable same-SoC/shared-IP and upstream
comparison before a driver change. This packaging change changes no driver or
board value. Existing touch processing/licensing and physical acceptance gaps
prevent fallback retirement. Factory MAC/calibration handoff remains read-only;
no provider with demonstrated equivalent security is introduced. Internal
four-channel playback and separate stereo DisplayPort routes stay distinct.
