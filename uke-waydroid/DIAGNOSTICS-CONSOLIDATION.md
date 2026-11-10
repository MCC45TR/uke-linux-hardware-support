# Manual Vulkan diagnostic consolidation

The 0.1.0-9 integration candidate includes the existing manual Android Vulkan
checker instead of requiring a separate diagnostics RPM. It adds no service,
kernel driver, runtime package name or dependency from Control to Waydroid.

The source worker reuses the exact qualified integration build 11262150 and
diagnostic build 11260540 SRPM inputs. Both complete archives, original recipe
hashes and source commits are pinned in `packaging/diagnostics-fold.json`.
The original Android NDK 27d API-30 headers, stubs and notices are retained;
neither a new NDK download nor a copied prebuilt executable is used. Native
compilation runs only in COPR. The recipe checks its complete inverse against
the integration parent and preserves the diagnostic build, installation and
ELF checks. Existing Binder SELinux policy, media preparation, configuration,
scripts and host tests are unchanged. The Android ELF exclusion remains limited
to its one Bionic executable; host-library dependencies remain automatic.

The command, overlay path, documentation and license paths remain the same.
Its four-KiB GPU fill and sixteen offscreen-frame comparisons remain manual
tests. They do not establish presentation, game compatibility, performance or
full Vulkan conformance. Userspace diagnostics cannot be moved into kernel/DT.

Before retiring the diagnostics producer, qualify the signed RPM and original
source ownership, compare host preparation and Android runtime behavior, check
normal DNF upgrade and rollback resolution, preserve signed recovery artifacts,
then verify COPR API and refreshed repository metadata absence. Source or build
success alone does not authorize retiring a runtime provider.
