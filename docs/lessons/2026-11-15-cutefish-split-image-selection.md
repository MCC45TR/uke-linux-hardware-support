# UKE-CUTEFISH-20261115: independent desktop selection

15 November 2026. Source and package-policy work on an x86_64 development PC;
binaries are built in COPR. No tablet installation or graphical result.

The current Rawhide COPR selection was `uke-cutefish-meta-1.0.0-37` and requested
the old monolithic desktop. The new selection requires the independently built
November 15 Cutefish set, native first-account setup and session configuration.
The standalone `uke-cutefish-meta.spec` emits release 38 without the shared
metadata source's KDE build requirements. The shared spec retains the same
Cutefish dependency policy for later integrated builds. KDE requirements and
files are unchanged; release 37's other installed selections are unaffected.

The provider project remains `mcc45tr/cutefish`; the image composer admits only
its named runtime packages under the existing pinned signing key. Runtime
isolation is checked against installed NEVRAs, including transitive KDE/GDM/
GNOME Shell dependencies. Native first-account setup must not be confused with
physical acceptance of login, touch/keyboard, GPU, portals or application UI.
Upstream supplies no Turkish Cutefish catalogs at this source lock; original
Fedora Turkish locale, fonts and complete supplied translations remain required.

Validation: metadata JSON, standalone spec parsing and source selection tests.
Signed RPMs, full DNF closure, first-account success/interruption and physical
Uke desktop acceptance require their own build and image records.

Follow-up: standalone release 39 requires desktop release 2 and session-config
release 2 after independent RPM ownership checks found the original routing
path duplicated. The upstream portal owns that path; GTK supplies its fallback.
The shared source spec advances to release 40 to avoid reusing either standalone
Cutefish NEVRA. That shared KDE-bearing source has not been rebuilt here.
Upstream advertises no Screenshot portal interface; native capture remains a
separate physical acceptance item.
