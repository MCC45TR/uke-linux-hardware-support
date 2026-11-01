# Packaging admission

Add distribution-native rules only after the component's Uke source and payload
contract is reviewable. Do not publish empty runtime packages as hardware support.

## GPU startup candidate — 14 October 2026

`make srpm-gpu` packages standard modules-load and modprobe configuration.
The matching boot recipe must omit the MSM module from its initramfs and remove
the global MSM blacklist. The qualified SQE, GMU and ZAP firmware must exist on
the real root before enabling this package. No startup script, KDE fork or
clock/voltage override is added.

KDE reported `freedreno / FD732` on the corrected 7.2.10 RAM candidate after
firmware installation. Remote access was subsequently lost; the cause remains
unknown. The automatic-start candidate has not been installed or boot-tested.
Keep the accepted kernel and boot route until repeated startup and sustained
rendering pass. Firmware scanout remains fixed at its existing nominal 60 Hz.
