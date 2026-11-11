# Adaptive integration ownership

12 November 2026. Control retains the Adaptive17 project plugin and public
display/DDC/tablet/suspend policy without rebuilding KDE or changing hardware
protocols. The existing three Control package names remain. The old standalone
Adaptive RPM is replaced only after source, native, signed-package, DNF,
configuration, activation and rollback checks.

The current Control Source0 archive is byte-identical to the historical
Control2.8.1-5 archive. Its three nested Adaptive archives and complete source
inventory are authenticated before reuse. All original plugin/parser, DDC,
udev and tmpfiles checks remain. Existing Control sources and native checks
are retained unchanged.

The project KSysGuard plugin is optional outside a KSysGuard installation.
Only that ELF path uses a dependency filter. Every one of its native ELF
requirements is declared as a rich requirement conditional on ksystemstats;
the native generator must match the complete declared set. Original KDE
components are required for Plasma consumers. GNOME and console installations
do not acquire KDE merely to install Control. Published RPMs and fresh KDE and
minimal dependency resolutions must independently verify this boundary.

Configuration paths, noreplace flags, licenses and documentation stay at their
previous locations. A private, bounded transaction record preserves the old
target's enabled, masked and active state across Obsoletes removal. New masks
take precedence; an inactive target is not started. This adds no runtime worker
or polling loop. It does not qualify physical automatic brightness, DDC,
suspend, GPU performance or original SensorProxy parity.

Retain the signed old Control3 and Adaptive17 recovery artifacts until supported
upgrade, reinstall, fresh and rollback resolution and device checks pass. Inspect
all outputs and source/recovery references before deleting a legacy build.
Producer absence and an excluded-name solver run do not establish that every
historical RPM is absent from refreshed repository metadata.
