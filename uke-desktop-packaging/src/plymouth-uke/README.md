# Uke Plymouth theme

17 November 2026. Target: Fedora Rawhide AArch64, POCO Pad X1 and attached displays.

The artwork and script originate from
[`Nabu-Fedora-Rawhide-Builder` at `67b6bb2398195bd026dae7e6258681a9c3420f08`](https://github.com/MCC45TR/Nabu-Fedora-Rawhide-Builder/tree/67b6bb2398195bd026dae7e6258681a9c3420f08/packaging/copr/senemos-nabu-plymouth/source).
`LICENSE.Nabu` retains its MIT copyright and permission notice. Xiaomi and
Fedora artwork retain their original identities; this package claims no
trademark ownership or endorsement.

The archived source SHA-256 is
`f2869dd426dc7f0c2f7c7f8d02852f0eb4293661dad0dcb801f22d64b5f718ec`.
The five PNG assets are unchanged. A new native theme plugin uses the public
Plymouth splash SDK and cached DRM connector metadata. It fits the logo to
23 mm, progress bar to 44 mm and Fedora artwork to 32 mm, independently for
each display. These are artwork sizes, not assumed monitor dimensions.
Device scale and quarter-turn rotation are accounted for before rendering.
The old script remains in source as an attributed reference, not a runtime provider.

Physical dimensions come from the original DRM driver (DT or EDID), never a
monitor diagonal supplied by the user. A 43-inch 4K display therefore uses
fewer pixels for the same physical artwork than the tablet. Missing dimensions,
contradictory equally sized heads and unknown renderer geometry use a conservative
96-DPI fallback; centimetre accuracy cannot be claimed in those cases. Identical
mode/size clones are accepted. Different physical sizes sharing an indistinguishable
Plymouth head cannot be resolved through the public SDK and remain a limitation.
Very small viewports fit the artwork to avoid clipping.

Boot progress is event driven. Shutdown/reboot animate only the small progress
rectangle at 8 Hz; timers stop on idle, hide and exit. Connector metadata is read
only on display addition/removal, without forced EDID detection, modesetting,
DRM master acquisition, private renderer layouts or background polling.
Offline-update modes, messages, questions and masked password prompts use the
standard splash callbacks. Boot console output stays in the journal rather than
being painted over the theme. Firmware and package update providers stay Fedora's.

The existing package name is `uke-plymonth`. Core recommends it through RPM
weak dependencies. Install/upgrade selects this theme when the current choice
is a distribution default; a custom theme remains selected. Removal resets the
selection only when this theme is selected. Distribution service conditions,
including `plymouth.enable=0`, remain authoritative. The four service drop-ins
use Plymouth's standard `--graphical-boot` option. They do not force a global
device scale or edit the kernel command line. A system-update target drop-in
wants the original Plymouth start unit for Fedora's offline-update path.

The original Fedora daemon, libraries, label plugin and systemd units are
dependencies. This is a plugin inside the existing theme RPM, not a Plymouth
fork or a new integration daemon. The original SDK interfaces are GPL-2.0-or-later;
the plugin uses that license, and the assets retain their MIT attribution.
No boot writer or Python payload is installed. The existing minimal kernel
initramfs explicitly omits Plymouth; early splash requires a separately reviewed
cumulative kernel integration and signed RPM delivery. Root theme installation
alone cannot provide pre-root splash.

COPR checks compile the actual plugin against original Fedora libraries and
exercise production layout on tablet/4K geometry, three device scales, rotation,
missing and ambiguous dimensions, small displays, all update modes, password
masking and timer cancellation with GCC/Clang and sanitizers. These checks do
not establish physical display handoff, shutdown or offline-update acceptance.
