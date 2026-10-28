# Uke hardware update, 27 October 2026

28 October correction: hardware `1.8.4-3` admits native touch standby only
when exactly one connected DSI-1 connector is explicitly disabled. Enabled,
missing, malformed and ambiguous panel states still require normal readiness.
Metas `1.4.0-3` selects that hardware revision. This supports external-only boot
health without claiming that a sleeping touchscreen has been physically tested.

Target: existing Uke installations on Fedora Rawhide AArch64 with
`uke-core-meta` and the selected Uke COPR repository enabled.

```sh
sudo dnf update --refresh
```

The matching Core selection requires Linux `7.3.0-33` or its matching
singleboot profile and the `uke-sound` family. The hardware family requires
BlueZ, libinput and `uke-libwacom`; these are pen/keyboard prerequisites.
Focus Pen THP processing and Focus Keyboard transport/authentication are not
qualified by installing these packages. KDE and Qt use original Fedora packages.

Every non-kernel source family and binary package uses the `uke-` prefix.
PipeWire, speaker orientation, the generic speaker preset and UCM/desktop
speaker controls are children of `uke-sound`. Material decoration becomes
`uke-material-decoration` under `uke-extras`. Original package capabilities
and versioned obsoletes preserve ordinary upgrade resolution. Frozen source
RPMs retain their hashes in the permanent source archive; binaries are rebuilt
from those recipes, not repackaged.

The kernel preserves touch identities, confirms new contact groups, retains
interior electrode coordinates and filters held single-contact patch roll.
A separate RAM module on POCO Pad X1 with kernel `7.3.0-31` improved gestures,
normal taps and deliberate dragging. The complete new kernel needs independent
boot acceptance. The desktop top-left selection defect remains open.

Disabling the optional KWin Touch Points visualization removed the observed
multifinger rendering stall. This preference does not change desktop selection
or input processing. A browser's ten-contact display is not a native input
acceptance test.

On the test tablet, a saved closed-lid output setup enabled the internal panel
and disabled DisplayPort. Correcting that setup through the original KScreen
interface retained DisplayPort with DSI disabled on a repeated software lid
transition; a webcam confirmed the external image. This is a per-session
layout correction, not a modified KDE package or a physical-cover acceptance
result. No EDID-specific layout is distributed to other devices.

Package availability, signatures, upgrade resolution and physical device
acceptance are recorded separately. Existing guarded boot activation and
rollback policies remain authoritative. Device-private calibration is not
included in these packages.
