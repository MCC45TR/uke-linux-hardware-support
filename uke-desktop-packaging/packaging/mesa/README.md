# Mesa AArch64 and Adreno 732 corrections

The exact Fedora `mesa-26.2.4-1.fc46` source RPM is pinned in
`manifests/native-runtime.json`. One upstream C++ conditional admits AArch64 to
LLVM host-feature detection. Disabled features must override CPU-name defaults:
the physical Uke llvmpipe crash stopped at an SVE instruction while Linux
reported no SVE. KDE applications and the original Fedora driver selection,
APIs and build flags are unchanged.

The build worker runs the exact patched feature function against LLVM's actual
feature map, checking every enabled and disabled attribute. The unpatched
AArch64 branch must fail this fixture. Signed payload, offline DNF rollback,
physical Wayland stability and observed output remain separate gates.

The Adreno 732 correction keeps Mesa's existing `0x43030b00` three-CCU profile,
changes its label from `FD735` to `FD732`, and admits the kernel's fuse-ID
wildcard. It does not alias another GPU or change shader properties. Native
shader/readback and KWin hardware rendering were observed with the experimental
kernel at 255 MHz on 13 October. This Mesa revision still needs its own package
and tablet tests; Vulkan, higher GPU frequencies and suspend remain unqualified.

Unavoidable upstream host tools: Meson `1.12.1-2.fc46`, Python development/runtime
`3.15.0-1.fc46`, and LLVM development libraries `23.1.2-1.fc46` are pinned in
the derived recipe. Mesa's upstream Mako `1.4.1-2.fc46`, PyYAML `6.0.3-6.fc45`
and Pycparser `3.00-4.fc45` source generators
run only in the build worker; replacing them has no practical narrow alternative.
Actual worker dependency versions stay in COPR logs. Every binary subpackage is
checked for Python files/shebangs before packaging and needs independent native
payload/dependency admission before delivery. Detailed physical evidence remains
in the private documentation journal.

## Render-only primary node candidate — 21 October 2026

Current Fedora `mesa-26.2.5-1.fc46.src.rpm`, SHA-256
`38209e85ce96607b65b0b60109b4e3b6959c75ad1e04e5ebfcd422ab3755c7e5`,
retains the reviewed Turnip primary-node open and common WSI behavior. On the
tablet running kernel 14/Mesa 26.2.4, bounded native tracing of the selected
Freedreno ICD shows `DRM_IOCTL_MODE_GETRESOURCES` returning `EOPNOTSUPP` on the
render-only primary device. The display controller is a separate DRM device.
Common WSI translates that failed resource query to host-memory exhaustion;
the same run has no measured memory pressure. Surfaceless EGL initializes
FD732 successfully. This does not qualify Vulkan presentation or performance.

Candidate `0003` probes the opened primary node before transferring its fd.
Only `EOPNOTSUPP` selects the existing no-display path. Valid KMS resources are
freed, rejected fds close once, and other failures retain the old path. It does
not choose another card, suppress extensions or alter GPU/clock properties.
The upstream MIT source attribution remains unchanged.

`bash tests/turnip-display-fd.sh SOURCE_DIRECTORY` compiles the exact patched
branch with host fakes. Disabled-display, failed-open, valid-resource,
unsupported-KMS, ENOMEM, EIO, EACCES and ENOTTY cases check ownership and failure
preservation. Complete Mesa compilation, signed coherent COPR integration,
DNF deployment and separate Vulkan/Wayland tablet tests remain required.
Source family `uke-additional` Release 4 includes this candidate. The fixture
accepts `--patched` for its already prepared build tree. Native builds and
physical acceptance remain separate; no support claim follows from inclusion.

The host-feature executable is a non-installed Meson target linked to the real
Mesa CPU-capability producer and LLVM dependency. It preserves every enabled
and disabled LLVM attribute, then checks Mesa's detected NEON state and both
single-threaded guard branches. Missing LLVM-map and missing-guard mutations
fail independently. The target runs before the full build and again in `%check`.

## Adreno 732 UBWC hint candidate — 24 October 2026

On Uke with Linux `7.2.12-33` and Mesa `26.2.5-11.fc46`, the pinned native
Vulkan CTS reported three image-atomic/barrier comparison failures. Independent
default processes reproduced them. Disabling only `enable_tp_ubwc_flag_hint`
passed all three, retaining storage-image UBWC. Selected compute and basic
legacy/synchronization2 groups then passed 113 cases, with 21 unsupported and
zero failed cases. These are temporary parameter comparisons, not results
from an installed rebuilt driver or full API conformance.

Candidate `0004` clears this property in the shared device-info copy only for
chip `0x43030b00`, including fuse variants. Turnip and Gallium use this shared
result. X1-45, other GPU properties, original records and explicit debug
override precedence remain unchanged. No global no-UBWC or flush-all setting
is installed. Upstream Mesa's MIT attribution is retained.

`bash tests/adreno732-ubwc-hint.sh SOURCE_DIRECTORY` compiles the exact patched
function against host fakes. Exact/wildcard/fused IDs, neighboring IDs, unknown
devices, unchanged fields and debug precedence are checked; unpatched source
fails the fixture. The prepared build tree uses `--patched`; the source recipe
runs the fixture before the full build and again in `%check`. Full Mesa build,
signed coherent package delivery and tests of that artifact without temporary
environment overrides remain required. Detailed source/runtime evidence is in
workspace lesson UKE-GPU-L027.

28 October 2026: the Uke Mesa filesystem child supplies
`30-uke-adreno732-interop.conf` through the standard systemd user environment
generator. It appends the same disabled TP UBWC hint used by the host driver
to the session's feature list, including independently supplied Flatpak Mesa.
Existing feature options are preserved; administrator environment precedence
remains available. The upstream property documents a system-wide agreement
requirement for this hardware family. A host-only driver default cannot impose
that agreement on a sandbox's separate driver.

Assignment fixtures and the real host environment generator pass. On POCO Pad
X1, the separately loaded Flatpak GL driver reports FD735 while host Mesa
reports FD732. Both flag values produce the same bounded GLES validation
results, including six pre-existing failed scenes. This does not reproduce or
close the reported Pad 7 application/desktop corruption. The session-coherence
delivery and the reporter's exact application acceptance remain separate.

## EGL descriptor lookup candidate

27 October 2026: `0006` caches the exact character-node filesystem identity
when enumerating DRM devices. A matching live descriptor avoids a repeated
sysfs lookup after sandbox startup. Device, inode, link and change-time checks
reject stale/replaced nodes; unknown nodes retain `drmGetDevice2` and its
existing ownership/error behavior. `_EGLDevice` remains private; no public EGL
ABI or sandbox permission changes. Collabora's original MIT attribution stays
with `src/egl/main/egldevice.c`.

The Mesa 26.2.5 and
[reviewed upstream implementation](https://gitlab.freedesktop.org/mesa/mesa/-/blob/2c95c9030c315ded1bdae9bc80bd2337416f434d/src/egl/main/egldevice.c)
repeat device discovery. The actual-function fixture covers cached primary/
render descriptors, aliases, identity changes, removed nodes, failed refresh,
unknown/regular descriptors, software selection and metadata fallback/freeing.
It passes on x86_64 and AArch64 emulation; the original lookup fails the same
cached-node case. Run `bash tests/egl-device-node-cache.sh "$MESA_SOURCE"`.

On physical Uke/kernel19, the final process-scoped EGL variant passes original
Chrome156 FD732 GL with GPU Seccomp2, all sampled GPU threads filtered, equal
pixels/context restoration and81 successful real MSM submissions. Four live
renderer processes retain BPF and three-level PID namespace mappings. The
existing angleisbroken workaround is retained. This is an isolated-library
Wayland result, not default Chrome or full Mesa-package acceptance.

The same cached-node check is used before both DRI2 device setup and the
GBM-derived render-fd capability lookup; unknown/display-only descriptors keep
the original compatible-render-device route. Both actual callers are covered
by the fixture. The C++ client-discovery diagnostic lives at
uke-desktop-integration/tests/chromium-egl-warmup.cpp in the parent source tree.
It uses upstream's existing GPU-zygote markers, preserves one thread and fd
count, and does not change a syscall or sandbox permission.

This candidate is not wired into the source recipe or installed. An EGL-only
control/candidate comparison against original signed Gallium/GBM is complete.
Full signed default-path integration, browser video and hotplug/lifecycle
remain separate acceptance gates. Central records:
UKE-CHR-L047/UKE-VID-L087 in `uke-linux-docs`.
