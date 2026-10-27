# Four speaker hardware controls in KDE

27 October 2026. POCO Pad X1, Fedora Rawhide AArch64, kernel 7.3.0-31 and
original KDE KMix 26.08.2. The live KMix ALSA interface exposes TopLeft,
TopRight, BottomLeft and BottomRight Digital level controls separately.
Their range remains bounded by the kernel's amplifier gain limit.

Open **Speaker Hardware Controls** from the KDE application menu. It starts
the original Fedora KMix with the ALSA backend. The standard KDE audio panel
continues to control application volume and stereo balance through PipeWire.
Use KMix's Configure Channels action if a hardware slider is hidden.

The package supplies a card-specific original KMix view profile containing
only the four amplifier levels. Existing user profiles take precedence and
are preserved. Hardware route switches and rotation enums remain available
through KMix's channel configuration, while automatic rotation owns routing.

These are four amplifier levels, not four independent surround channels.
The qualified source route remains a stereo feed distributed to four speakers.
Hardware names describe the USB-C-right landscape reference. The active
Wayland display transform selects the left/right assignment through the
existing orientation helper. Physical movement with a locked display does not
override the compositor. Manual rotation enums in KMix are reconciled back to
the display by that helper; use display settings for rotation.

The launcher adds no autostart process, mixer restore service, volume change,
new kernel interface or target interpreter. It does not replace or rebuild
KMix, plasma-pa, KWin or a GNOME application. Four-way physical stereo order,
user-interface usability and suspend/resume remain separate acceptance tests.
