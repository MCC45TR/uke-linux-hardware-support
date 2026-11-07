# Speaker controls integration

6 November2026. Core1.0.7-20 retains the complete Core19.2 selection and absorbs
uke-speaker-controls0.1.0-5. The launcher, original KMix view profile,
documentation and license keep their existing paths and contents. Original
Fedora KMix is required only with plasma-workspace. A compatibility provide
satisfies existing consumers; the obsolete data-only RPM is removed during
normal DNF upgrade. No KDE or GNOME application is rebuilt.

Amplifier drivers, gain limits and protection belong to the kernel. KMix's
view profile and menu entry describe a desktop interface, so kernel/DT code
cannot replace those files. Consolidating them into the existing integration
package removes a separate RPM while preserving their function. The package
adds no daemon, autostart, mixer command, hardware write or scriptlet. Existing
user KMix profiles retain priority; volume and orientation preferences are
untouched. This does not establish working sound or speaker protection.

The source worker pins signed sourceCore19.2/COPR11251074 and validates its
trusted signatures. It extracts the actual final Core profile and original
LICENSE/README from their nested source inputs. Their hashes and the four
speaker metadata hashes match the installed counterparts. Every existing
requirement, recommendation, conflict and provide is retained. Native build
checks validate the unchanged content, desktop entry and XML. Sole-output,
RPM metadata/signatures, dependency solving, file ownership transfer and
preference preservation must also be checked during delivery.

Source preparation and RPM creation run in COPR. The target receives normal
versioned DNF updates; no locally compiled program or copied binary is used.
