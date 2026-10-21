# UKE-DESKTOP-013 — preserve real producers and narrow runtime failures

- Date: 21 October 2026.
- Environment: Fedora Rawhide AArch64 source/build services; separate x86_64
  Mesa-library fixtures and Linux 7.2.11-14 tablet diagnostics.
- Evidence: native build 11181620 completed compilation, then failed the Mesa
  feature fixture because upstream 26.2.5 now also reads `util_get_cpu_caps()`.
  No new binaries were admitted. Release 4 links the real Mesa producer through
  a non-installed Meson executable and runs it before the expensive full build.
  The host result preserves 115 LLVM features and both NEON guards; independent
  omitted-map and omitted-guard mutations fail. This is not AArch64 acceptance.
- Runtime findings: selected-Freedreno tracing on the tablet returns
  `EOPNOTSUPP` for KMS resources on its render-only primary node. Common WSI maps
  that failure to host-memory exhaustion. The narrow Turnip correction closes
  only that unsupported fd, preserves other errors and uses the existing
  no-display path. Eight actual-block ownership/error cases pass. It neither
  selects another card nor changes extensions, clocks, firmware or DT values.
  Original Fedora GLMark validation separately passes on FD732; the earlier
  offscreen zero-pixel result was a readback-harness limitation.
- Source comparison: the donor policy, SM7675 survey/acquisition inventory and
  hardware-support donor manifest were consulted. Xiaomi kernel
  `5712080a4b0c0b1a879e6d145c601c649ddaddfd` and the reviewed gen7_11_0 modules
  `ed4ad0d1ad00b755af2fe61387dcc5397ad20b8f` describe the Android/OEM graphics
  contract, not this native DRM query. Palawan/shared-IP references do not
  justify transferring GPU allocations or power settings. Mesa
  `ed729f3fab71e267326f3156c80026859377ec58`, Android comparison
  `4b10f49cd53b11321467620fc302b52ff00cddf5`, secondary gen8 comparison
  `9c7e022677dfa3abb356c2b6732cbd2e25783d01`, and current upstream
  `5783d30801332ff9d8334fa69b1dfbbabf1da2d3` retain the same unchecked primary
  open and WSI resource-failure mapping. Relevant files are
  `src/freedreno/vulkan/tu_knl.cc` and `src/vulkan/wsi/wsi_common_display.c`.
  Mesa's file-level MIT attribution is retained. No applicable donor fix was
  found in these inspected paths; the target ioctl and standard no-display
  path provide the bounded alternative evidence.
- Account enumeration: original Rawhide AccountsService 26.27.3-2.fc45 warns
  when optional TCB storage is absent. The correction ignores only `ENOENT`;
  actual-block root/unprivileged tests preserve enumeration, ENOTDIR and EACCES.
  No authentication directory, account database or PAM configuration is created
  or changed. The original recipe and complete signed source remain available.
- Packaging: all rebuilt additional children receive fresh revisions. Source
  tracking includes AccountsService and its host-only D-Bus mock pin. Service
  alias migration preserves existing unit state and requires a family revision.
  Original KDE applications and the successful power/sensor cohorts are retained.
- Uncertainty: complete native build, signed binary/ABI/solver checks and target
  DNF/API/Wayland/Vulkan regressions remain required. No recoverable new-kernel
  boot, physical display pacing, sensor calibration or audio result is inferred.
