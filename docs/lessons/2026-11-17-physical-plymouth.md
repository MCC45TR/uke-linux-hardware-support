# UKE-PHYSICAL-SPLASH-20261117: distinguish pixels from physical display size

17 November 2026. Fedora Rawhide development PC and POCO Pad X1 on
accepted kernel 7.3.4-8 with Control 2.17.0-27. The user accepts the gray-screen
fix and requests the established theme at a constant centimetre size on both
the tablet and an attached monitor, including a 43-inch 4K display.

The original script theme exposes viewport pixels but no per-head physical
dimensions. A hard-coded tablet size or forced global scale would fail on a
different monitor. The original Fedora Plymouth 26.134.222 SDK exposes the
renderer and display buffer but keeps head connector identity opaque. The
upstream DRM renderer consumes connector millimetres and handles rotation.

The new theme plugin remains inside the existing uke-plymonth RPM and uses
the unmodified Fedora engine, public splash SDK and libdrm cached metadata.
It matches the current viewport, rotation and device scale against connected
outputs. A unique physical density, or equal-density clones, provides the
centimetre layout. Missing dimensions and ambiguous equal-resolution heads
retain a documented fallback instead of claiming a measured physical size.
This cannot resolve different-size clones sharing one opaque renderer head.
No mode is set and no EDID detection or periodic connector probe is requested.

The original attributed assets, offline-update modes and secret prompts remain.
Shutdown animation damages only the bar at 8 Hz and stops on idle/hide/exit.
Normal boot progress is event driven. Existing journal diagnostics remain.
The kernel's current minimal initramfs omits Plymouth; root installation alone
does not qualify an early splash. That requires cumulative kernel integration.

Host checks are limited to recipe/text/schema review. COPR compilation against
the original libraries, GCC/Clang sanitizer checks, signed RPM qualification,
DNF, optical boot/shutdown and a real offline-update run are pending. A test
geometry for a 43-inch monitor is a software fixture, not a physical monitor test.

Primary references: [upstream release](https://www.freedesktop.org/software/plymouth/releases/plymouth-26.134.222.tar.xz),
archive SHA-256 `6ee1068216c5047923fabf0f6aa583e9d77cab383077358df3fc22f2ee398bb0`,
`src/libply-splash-core/ply-{renderer,pixel-display,pixel-buffer}.h`,
`src/plugins/renderers/drm/plugin.c`, and
`src/libply-splash-core/ply-boot-splash-plugin.h`.

COPR 11342617 stops in source export because custom-source builds require an
exported recipe rather than a directory containing only an SRPM. The corrected
worker extracts that generated SRPM without compiling on the PC or tablet.
Build 11342626 then reaches native compilation and rejects the original SDK's
C-only enum forward declaration through ply-label.h. Release 2 isolates that
header in a small C adapter; the theme stays C++, the SDK remains unchanged,
and compiler diagnostics are retained.
