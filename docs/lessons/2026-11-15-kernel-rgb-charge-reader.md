# UKE-RGB-CHARGE-20261115: keep status readers compatible with native policy

15 November 2026. The running Uke kernel is7.3.3-28-unstable and the fresh
Fedora image contains Control/TUI16. The pending native kernel appends
charge-complete mode6 to ABI1 without changing ioctl numbers or structure
sizes. The old reader rejects every mode above5, so a valid kernel completion
would become a client error. Hardware timing and charge policy remain kernel
owned; userspace needs only a bounded status label and enum admission.

The candidate adds that label to the existing Control reader and updates its
private-bus fixture to accept6 and reject7. All prior status-bound and write
assertions remain. It adds no runtime package, timer, hardware write or daemon.
The three actual source preimages in the signed Control19 source match the
prepared changes. Fresh COPR history found successful19/11328227 after the
installed16; using16 as the new package parent would drop the Motion changes.
The new recipe therefore preserves every19 input, preparation, check,
scriptlet and file owner, with a complete recipe inverse. Control/TUI20 stay
paired; the package exports `uke-control(rgb-charge-status-v1)` for delivery
dependency closure.

The COPR source-worker SRPM for19 is unsigned. Its signed Pulp source RPM is
18471e5b, recipeb9835d61, with both expected2204 signatures verified in an
isolated filesystem keyring. Extracted source files compare equal even though
the compressed RPM payload digests differ after the native source repack.
Do not equate an unsigned source-generation file with the signed native result.

PC checks cover source identity, recipe inversion and text only. Native private
bus/build tests, signed RPM output, DNF dependency resolution and actual device
reader/LED behavior remain pending. A successful reader build cannot establish
physical notification timing, charging cutoff or either camera.

The first request11331618 passes trusted parent signatures, complete recipe
inverse and source retention, then returns a prebuilt SRPM to the custom
source method. The collector requires the spec and source inputs and reports
no spec; its source_package remains null and no native build starts. Return
the reviewed spec and68 sources directly. This fixes the source transport,
without changing reader code or consuming a published Control20 version.
