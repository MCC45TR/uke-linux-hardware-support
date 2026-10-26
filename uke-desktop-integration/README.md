# uke-desktop-integration

Uke rotation, cover, brightness, power and audio desktop policies for Xiaomi Pad 7 / POCO Pad X1 (`uke`, SM7675).

This is an initial project-owned source and packaging repository. The manifest
lists the intended packages, Uke evidence gates and current build readiness.
The desktop policy package has no functional payload or device acceptance at
this checkpoint.

## Graphics diagnostic

On 18 October 2026, the standalone `uke-vulkan-probe` passed a bounded Vulkan
transfer test on Uke with Linux `7.2.10-23` and Mesa `26.2.4`. This result applies
to the diagnostic only; the desktop policy package remains a scaffold.

Build with `make vulkan-probe` using a C++17 compiler, Vulkan headers and loader.
Run `build/uke-vulkan-probe` as the desktop user. The probe requests no display
extensions, submits a 4 KiB GPU fill, waits up to five seconds for its fence and
checks all 1024 returned words. Software adapters cannot produce a passing
result. Use an external process timeout when investigating a driver hang.
This does not test presentation, shaders, browser integration or video codecs.

`tests/browser-gpu.html` checks canvas pixels, a WebGL2 shader, controlled
WebGL context loss/restoration, a WebGPU canvas triangle and 64 compute rounds
with 4096 words each. On 24 October 2026, original Fedora Chromium
154.0.8037.92 passed 30 fresh runs with alternating window sizes over 120.953
seconds through Xwayland with its Vulkan feature enabled. All 7864320 compute
values and 30 raster/context checks passed; maximum sampled temperature was
46.7 °C and GPU process crash count was 0.

The default GL backend returns no WebGPU adapter because of exact Chromium
Linux backend policy. The tested desktop-user launcher now selects
`--ozone-platform=x11 --enable-features=Vulkan`, retaining ANGLE OpenGL with
GaneshVulkan. Its installed command passed again with a temporary profile.
Direct invocations without these options retain distribution defaults.
The Wayland/Vulkan compatibility warning is avoided by this Xwayland path.
GPU-process sandboxing, long-term endurance and physical visual/input quality
remain open. Early GPU-sandbox options failed acceleration and were excluded.
The earlier smaller tests remain under `referances/`. This component's RPM
does not install a browser policy.

Source pins and compatibility limits are recorded in
[`manifests/graphics-research-20261018.json`](manifests/graphics-research-20261018.json).
The canonical work order is in the workspace documentation at
`docs/testing/GPU-BROWSER-SUPPORT-2026-10-18.md`.

On 24 October 2026, Uke Linux `7.2.12-31` and Mesa `26.2.5-11` passed the
native Vulkan transfer check and separate 600-frame Wayland/XCB vkcube runs.
The standalone `uke-gles-probe` passed exact-color rasterization and a
1024-word GLES compute/readback check on Freedreno and, separately, Zink over
Turnip. Build it with `make gles-probe` and EGL/GLES development libraries;
run it with `EGL_PLATFORM=surfaceless` and an external process timeout.
It also reports mediump/highp pixels without treating those diagnostic values
as a conformance result. Forced llvmpipe returns 2 instead of hardware success.

`make opencl-probe` requires OpenCL headers and the ICD loader development
library. After installation of the matching signed `uke-mesa-libOpenCL`
package and its dependencies, `uke-opencl-probe` verified 1024 integer GPU
results on FD732. Its extended test then exposed a real build failure with
Fedora's original libclc22 SPIR-V libraries. The pinned Mesa-maintained
replacement passed 1024 integer and 4096 bounded floating-point results;
maximum absolute error was `2.38418579e-07`. A CPU-only provider returns 2.
Full OpenCL API and numerical conformance remain untested. No global renderer,
precision or `RUSTICL_ENABLE` override is installed by these probes.

`make mesa-libclc-srpm` builds the source RPM for
[`uke-mesa-libclc22-spirv`](packaging/uke-mesa-libclc22-spirv.spec) from the
hash-checked Mesa-libclc LLVM 22 branch. Signed AArch64
[COPR build 11202917](https://copr.fedorainfracloud.org/coprs/build/11202917)
passed an isolated Uke comparison before installation. Only the Fedora
`libclc22-spirv` subset was replaced; the parent `libclc22` and its other target
libraries were retained. Compatibility is restricted to Fedora
`22.1.8-4.fc46` and needs review when that base changes. No repository channel
was enabled. The upstream license and credits are included in the package.

The current work order is `docs/testing/GRAPHICS-VALIDATION-2026-10-24.md`;
dated source, host and tablet evidence is recorded separately in
`docs/lessons/2026-10-24-UKE-GRAPHICS-VALIDATION.md` in the workspace knowledge
repository. The desktop policy package remains a scaffold.

The pinned Khronos GLES CTS build separately passed 64 precision, 625 loop and
41 basic compute cases on FD732 on 24 October 2026: 730 functional passes with
no failures, unsupported cases, warnings or waivers in those groups. Six
information cases are counted separately. These bounded surfaceless tests do
not establish complete API conformance, native Vulkan CTS, presentation or
endurance. Source, target audit and QPA receipts are tracked in workspace
lessons UKE-GPU-L019–L022. After a user-reported restart, the same 730 cases
passed again on Linux `7.2.12-33`, alongside native OpenCL/GLES/Vulkan/Zink
baseline checks. The restart recheck and separate native Vulkan CTS
preparation are recorded in UKE-GPU-L023–L025.

The dedicated native Vulkan CTS subsequently found three image-atomic/barrier
comparison failures on `7.2.12-33`/Mesa `26.2.5-11`. Disabling only the TP UBWC
flag hint passed those reproductions and 113 selected compute/synchronization
cases, with 21 unsupported. A chip-specific shared Mesa source correction is
prepared; its rebuilt artifact remains unqualified. These temporary parameter
comparisons and the open GPU sandbox/media work are recorded in UKE-GPU-L027–L028.

## Package scope

- `uke-desktop-integration`

Nabu reference families: `nabu-desktop-integration`.
These are process references; Uke wiring, firmware and runtime behavior require
independent implementation and validation.

## Required validation

- Use measured Uke input and sensor paths.
- Require physical suspend, charger and routing evidence.
- Keep generic distribution defaults until behavior is measured.

The first packaging target is Fedora Rawhide AArch64 in
[uke-linux-test COPR](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-test/).
DEB and Alpine APK targets require their own native rules later.
Unimplemented targets fail explicitly rather than producing placeholder RPMs.

Use `make validate` to check this repository's manifest contract. Native device
applications use C++; upstream C/assembly interfaces retain their original
languages. Host automation uses Bash or Make. Python does not enter tablet
payloads. Source archives belong in `referances/`, development in `src/`.
Public records exclude personal paths, unit identifiers and calibration data.

See [the roadmap](docs/ROADMAP.md), [source policy](docs/SOURCE-POLICY.md) and
[manifest](manifests/component.json). New material findings are recorded in
`docs/lessons/`; source, package, emulation and physical evidence stay distinct.
