# Video reference inputs

Radxa libva-v4l2 commit `6d03c3f8fb19b86f84b2b6c60ff20d1402f0ac41`:

- Source: https://github.com/radxa-pkg/libva-v4l2
- Acquisition: GitHub API tarball for the exact commit, 25 October 2026.
- SHA-256: `e8756bf642eb3fc458209b8dd2380571dcb52ec4b2201e9d09db208b7e995d52`.
- Build source: the byte-identical permanent `copr-migration-archive-20261020`
  release asset, retained on 27 October after the API returned different bytes.
- License: MIT; included Linux kernel patches retain GPL-2.0-only.
- Upstream installer and Python distribution helper were not executed.
- Development uses the component patch; the acquired archive remains unchanged.

The kernel source candidate, exact Uke OEM and same-SoC comparisons belong to
the kernel component. Firmware is excluded from this public source package.

30 October 2026 automatic policy review:

- Current qcom-va-driver head remains `0980a1decf26d34c9aa71cd0a4a0d25703a7a17f`.
  Its decoder memory-budget/failure changes are outside this policy update;
  no reviewed upstream native per-user D-Bus activity interface replaces the
  new project-owned client. The pinned archive is not silently advanced.
- The client and session service use freedesktop D-Bus and Power Profiles;
  no OEM board values or donor scripts are copied or executed. Existing Uke
  kernel54 cache/budget evidence is unchanged. All new native builds are remote.
