# Video reference inputs

Radxa libva-v4l2 commit `6d03c3f8fb19b86f84b2b6c60ff20d1402f0ac41`:

- Source: https://github.com/radxa-pkg/libva-v4l2
- Acquisition: GitHub API tarball for the exact commit, 25 October 2026.
- SHA-256: `e8756bf642eb3fc458209b8dd2380571dcb52ec4b2201e9d09db208b7e995d52`.
- License: MIT; included Linux kernel patches retain GPL-2.0-only.
- Upstream installer and Python distribution helper were not executed.
- Development uses the component patch; the acquired archive remains unchanged.

The kernel source candidate, exact Uke OEM and same-SoC comparisons belong to
the kernel component. Firmware is excluded from this public source package.

FFmpeg flush review, 27 October 2026 local date:

- Official mirror commit: `d31915e9dce4d33676fadc6c3e08965b4acc132b`.
- File: `libavcodec/v4l2_m2m_dec.c`, retained under `ffmpeg-flush-20261027/`.
- SHA-256: `a035dacdc945f296007216acc549446fe8350e14799a2e19012d498149519f37`.
- The file is byte-identical to the retained 9.0.2 decoder and still has no
  flush callback. This is a source observation, not a tested seek result.
- Source URL: https://github.com/FFmpeg/FFmpeg/blob/d31915e9dce4d33676fadc6c3e08965b4acc132b/libavcodec/v4l2_m2m_dec.c
- License: LGPL-2.1-or-later. No acquired source is modified or executed.
