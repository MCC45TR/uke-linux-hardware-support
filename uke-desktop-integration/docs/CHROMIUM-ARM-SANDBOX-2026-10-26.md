# Chromium156 Linux ARM GPU sandbox candidate

Date: 26 October 2026. Target: POCO Pad X1, Fedora Rawhide, kernel7.3.0-15,
Mesa26.2.5 and Chrome156.0.8078.47. This source candidate has no full build,
package or device acceptance yet. The original Google Chrome stays installed.

Normal Chrome produces correct FD732 GL pixels with GPU Seccomp0. Its early,
fatal sandbox comparison has Seccomp2/one filter but hardware GL initialization
fails. Diagnostic file tracing observes renderD128 and GLVND EGL vendor-directory
opens failing through the file broker after BPF activation. Both GPU reports,
live process snapshots and complete private trace are retained separately.
Ptrace launch environment contamination is excluded from acceptance.

The patch targets Chromium revision1d17a2e2ee147bb7d6ae2ab6abdb3a2510c17775,
content/common/gpu_pre_sandbox_hook_linux.cc. It uses the existing DRM helper
for ordinary Linux ARM, adds read-only Mesa vendor discovery paths and preloads
Mesa EGL before BPF. The obsolete standalone libglapi becomes an optional
preload instead of a condition for loading Mesa DRI. Mali, Tegra, ChromeOS and
Cast selections retain their existing branches. No EGL display/context is
created in this hook. Full startup thread behavior still needs device proof.

The host fixture compiles the actual changed functions with stub broker and
loader boundaries. It covers ordinary ARM versus other architectures/platforms,
exact Mesa discovery paths, missing libglapi and Mali/Tegra isolation. Patch
application and the fixture pass against the pinned source. It does not compile
Chromium, execute seccomp, open real DRM devices or establish driver safety.

Fedora packaging is pinned at e402dd33400be892fc94d8c1f2fab15c9dc81c9b,
which packages exactly156.0.8078.47. Its AArch64 defaults select V4L2 and disable
VA; the experimental build must explicitly select VA for the installed V4L2
VA backend. Retain original distribution packaging and a distinct test release.
Do not replace the existing Google Chrome profile or default-browser setting.

Unmodified upstream GN/bootstrap and generated-code Python tools are build-host
inputs, pinned by the Chromium source archive and Fedora packaging revision.
They are needed by the upstream build graph; rewriting that graph has no
practical small alternative. They must not be invoked on the tablet or shipped
as project-owned Python payloads. Audit the built RPMs and runtime dependencies
before delivery; a source-file list alone does not prove the resulting payload.
Fedora acquisition/cleaning scripts are research inputs and are not executed.

Remaining validation: full native AArch64 build; signature and payload audit;
controlled signed DNF delivery; exact GPU before/after reports; GPU Seccomp2
with FD732 clear/shader/context-restoration pixels; native and VA codec sessions;
WebCodecs/WebRTC output and actual VPU evidence. Video node, DMA-BUF and fence
permissions require their own traced evidence. No global blocklist bypass,
sandbox disable, forced DrDC or change to Mesa angleisbroken is introduced.

The second patch resolves Iris nodes from kernel-owned sysfs driver and exact
decoder/encoder names, only for the enabled acceleration direction. It reads
metadata before BPF and does not open camera/touch nodes. Renumbered video19/23,
wrong driver, unrelated name, overlong/missing identity and disabled-direction
fixtures pass. This covers the backend constructor reopening a stored device
path; actual sandboxed sessions remain untested.

The verified clean archive contains the identical pinned GPU-hook source.
Both patches apply together and both extracted-function fixtures pass.
Source RPM156.0.8078.47-1.1.uke2.fc46 has SHA-256
65d0869d18c6829f8eb819142a49b608a1f53e4acd922a6838b636fd2d2aac6d.
Expanded AArch64 packaging selects use_vaapi=true without V4L2 codec flags.
The retained Fedora spec delta records the exact release, flags and patch
application. This is an SRPM, not a completed Chromium build or tablet result.

COPR11208913 accepts the single AArch64 submission in the separate
chromium-sandbox-20261026 test directory. Native compilation and binary
RPM inspection remain pending. Fedora's original chromium.conf includes
feature forces and an X11-only global blocklist/workaround bypass; invoke
the candidate binary directly with reviewed arguments for the controlled
trial. Do not run its distribution wrapper as the baseline or promote its
configuration before separate device qualification.
