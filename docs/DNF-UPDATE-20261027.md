# Uke hardware update, 28 October 2026

28 October correction: hardware `1.8.5-1` admits a connected enabled DSI,
DisplayPort or HDMI output during boot health. Native touch standby is valid
only when exactly one connected DSI-1 connector is explicitly disabled.
Enabled, missing, malformed and ambiguous panel states still require normal
touch readiness. Metas `1.4.0-4` selects this revision. Dormant input and
physical display accuracy are not inferred from these health checks.

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
normal taps and deliberate dragging. On 28 October, the complete signed
`7.3.0-33` kernel was installed through DNF, staged through the guarded updater,
rebooted and accepted with matching modules. New-kernel physical touch
qualification remains separate. The desktop top-left selection defect remains open.

Disabling the optional KWin Touch Points visualization removed the observed
multifinger rendering stall. This preference does not change desktop selection
or input processing. A browser's ten-contact display is not a native input
acceptance test.

On the test tablet, a saved closed-lid output setup enabled the internal panel
and disabled DisplayPort. Correcting that setup through the original KScreen
interface retained DisplayPort with DSI disabled on a repeated software lid
transition; a webcam confirmed the external image. This is a per-session
layout correction, not a modified KDE package or a physical-cover acceptance
result. After reboot on `7.3.0-33`, a repeated software transition retained
external output with DSI disabled; the webcam showed the external image.
Hardware `1.8.5-1` also passed boot acceptance in this external-only state.
Normal idle blanking is separate from the lid policy. No EDID-specific layout
is distributed to other devices.

Native Rawhide AArch64 builds succeeded in both channels:

| Source | Version-release | Test build | Promotion build |
|---|---|---|---|
| Kernel | 7.3.0-33 | [11216502](https://copr.fedorainfracloud.org/coprs/build/11216502) | [11216516](https://copr.fedorainfracloud.org/coprs/build/11216516) |
| uke-core | 1.15.1-3 | 11216543 | 11216572 |
| uke-hw | 1.8.5-1 | 11217721 | 11217723 |
| uke-power | 1.3.3-4 | 11216545 | 11216574 |
| uke-sensors | 1.0.9-5 | 11217417 | 11217421 |
| uke-extras | 1.0.4-2 | 11216547 | 11216576 |
| uke-additional | 1.0.0-13 | 11216571 | 11216577 |
| uke-metas | 1.4.0-4 | 11217722 | 11217724 |
| uke-camera | 1.0.0-3 | 11216550 | 11216579 |
| uke-sound | 1.0.5-1 | 11217418 | 11217422 |

Each channel's 108 binary RPMs passed trusted signature, payload digest,
exact source-family, architecture, prefix, Python-payload and file-ownership
checks. Normal tablet DNF updates installed the cohort without solver escape
flags. Qt GUI and Dolphin updates were held by an installed Ayugram dependency
on Qt 6.11 private API; the hardware update completed successfully. Automatic
idle suspend subsequently returned `EBUSY`, and touch gesture-entry attempts
logged `-110` with power retention refused. Suspend and double-tap wake are not
qualified by this release's boot and display results.

The final COPR inventory contains nine non-kernel source families per channel,
plus the existing kernel profiles. Twenty-nine retired standalone definitions
were removed after recording their metadata and retaining available historical
source RPMs. Some historical source URLs were already unavailable before
retirement; source archival completeness is not claimed.

Package availability, signatures, upgrade resolution and physical device
acceptance are recorded separately. Existing guarded boot activation and
rollback policies remain authoritative. Device-private calibration is not
included in these packages.
