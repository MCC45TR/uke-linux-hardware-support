# Chromium156 Linux ARM GPU sandbox candidate

Date: 26 October 2026. Target: POCO Pad X1, Fedora Rawhide, kernel7.3.0-15,
Mesa26.2.5 and Chrome156.0.8078.47. Native compilation, RPM generation and
trusted signature/payload checks pass. Device acceptance remains open;
the original Google Chrome stays installed.

The first native candidate fails the kernel16 early/fatal GL regression:
Seccomp2 is active, but the hardware GPU crashes and the replacement disables
GL. Follow-up source/SRPM changes pass host checks. The user subsequently
stopped separate Chromium builds: COPR11210239 is canceled, both11208913 and
11210239 are deleted, and the Chromium package record is removed. Neither
candidate has accepted sandboxed hardware rendering/video. Retained patches
are research records, not an active browser delivery route. Continue integration
against the original distribution/browser packages and existing Uke families.

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

Remaining validation: target ABI/transitive dependency and DNF-plan audit;
controlled signed delivery; exact GPU before/after reports; GPU Seccomp2
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
application. The SRPM receipt is distinct from the later native result below.

COPR11208913 succeeds at12:25:45UTC in the separate
chromium-sandbox-20261026 test directory. Full AArch64 compilation, link and
RPM generation complete. Both browser/common packages pass isolated signature,
header and payload checks against DAFC3C5A881FB49C7167EE2D6F772E3D487BD13E.
No Python payload or direct dependency is included. The original SELinux
semanage/restorecon scriptlet is retained; target dependency origins and the
GLIBC2.45/libavcodec63 ABI still require a fresh target audit. RPM SHA-256:

- chromium: b4c4a12c8e2b4d6a83fa826a4d3049d8a7b27cb4f33e51e347e58aef27169418
- chromium-common: 84245afd025d201d9ed3bb4494488496369566d171939e5dcbd582b22b7564c1
- Browser ELF: ae92d8d005cd68ea5bd550ce8f666573a0f3b81b9a5f70fdd870fd3b7d0c29da

Initial cached RPM URLs return404 after build completion; fresh-query downloads
provide the independently verified packages. No duplicate build is submitted.
The packages are not installed. Fedora's original chromium.conf includes
feature forces and an X11-only global blocklist/workaround bypass; invoke
the candidate binary directly with reviewed arguments for the controlled
trial. Do not run its distribution wrapper as the baseline or promote its
configuration before separate device qualification.

## Kernel16 failure and follow-up

On Linux7.3.0-16/sourcec5e9641, original Google156 passes FD732 GL pixels and
context restoration with a false sandbox report. The modified uke2 binary is
run from a verified user-owned extraction with the installed Google helper
available through a nonempty CHROME_DEVEL_SANDBOX. This does not disable the
sandbox or install Chromium. Its first GPU exits139; the replacement has
Seccomp2/one filter, NoNewPrivs1 and CapEff0 but disabled GL. The test fails.

Two concrete gaps remain. Mesa's modular `/usr/lib64/gbm/dri_gbm.so` open is
denied after BPF. Independently, Mesa's EGL-cache worker creation requests
SCHED_BATCH for a sibling thread; the existing target restriction terminates
the GPU. Actual Mesa26.2.5 u_queue.c ignores failure of this optional hint.
Current Chromium3a1201c8 has the identical GPU policy to pinned156.
Ptrace/errno-injection trials are diagnostic failures, not acceptance.

Patch0004 admits only that GBM file read and returns EPERM for SCHED_BATCH on
ARM64 Linux, excluding ChromeOS. It grants no scheduler operation. The actual
function fixture fails before the change and passes afterward, including
unchanged other-policy/platform cases. Existing Mesa/Iris fixtures still pass.
Patch0005 keeps privacy/Qt and stock graphics feature defaults with ARM64
early/fatal sandboxing; no encoder, zero-copy or global bypass force remains.
Its actual-configuration checks pass for ARM64/x86 and Wayland/X11.

The original Fedora SELinux policy already labels the packaged binary/helper
correctly on the tablet. Patch0006 retains restorecon over the browser tree,
removes obsolete local semanage rules/dependency and runs four native source/
configuration fixtures. The corrected uke3 source RPM contains all four;
digest QA catches an earlier conditional Source omission before submission.
SRPM SHA-256:
fb1a2e8f1fd28dd4e252742697ffbb91b5616ca6083c48ba46099636d865e38c.
COPR11210239 is canceled and deleted at the user's request. The incomplete
full build and its native checks are not passes. No replacement standalone
Chromium build is authorized. Original-browser sandboxed rendering and codec
acceptance remain open; package names created by the project must start with
`uke-` and remain children of their existing source family.
