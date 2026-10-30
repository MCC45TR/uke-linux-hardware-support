# Uke Tablet in KDE System Settings

30 October 2026: version 0.2.0 implements a native read-only KCM in the original
KDE System Settings. Fifteen component cards load independently with bounded
queries. Battery metrics retain producer units/validity and valid zero cycles;
unread, stale, denied, unsupported and unqualified states remain distinct.
Turkish translations, accessible labels and a fixture-only render harness are
included. No KDE application is copied or forked.

`make check` builds the plugin and runs model tests. The render fixture loads
the actual plugin through KCMUtils with synthetic observations and performs no
hardware query. Source, model/render, RPM payload, installed System Settings and
physical tablet acceptance remain separate. The source does not offer active
maintenance, charging, playback/recording or chroot controls.

Requires Qt 6.8+, KDE Frameworks 6, Kirigami and Control 2.4.0+. Fedora Rawhide is
the first packaging target. Other distributions require their own dependency and
native packaging acceptance. No Python target payload is introduced.

See the [API](../uke-control/OVERVIEW.md), [implementation decisions](../uke-control/REVIEW-2026-10-30.md),
[remaining gates](docs/ROADMAP.md) and [manifest](manifests/component.json).
COPR publication is disabled until exact source validation and package review;
a successful build does not establish installed or physical support.
