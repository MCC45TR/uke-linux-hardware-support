# Standard UPower provider preparation

11 November 2026. Core22 accepts `upower >= 1.91.5` instead of the private
`uke-upower` name. Both the current Uke package and the original Fedora package
provide that capability. This removes a dependency obstacle without replacing
the daemon, adding an Obsoletes rule, changing configuration or retiring a
provider. All other Core selection and the complete source archive are kept.

The actual signed Fedora daemon/library pair reads normal current-boot Uke
telemetry in an isolated private bus with read-only hardware sysfs. That is not
full parity for threshold error propagation, unknown/late capacity, history,
wireless accessories, critical actions, cold boot or firmware restart. The
current fork's exact remaining differences and their upstream return paths
remain tracked. Hardware protocols and power_supply lifecycle belong in the
kernel; a kernel change cannot correct a userspace D-Bus result or history bug.

`host/prepare-upower-provider.sh PARENT_SRPM OUTPUT_DIRECTORY` performs data-only
preparation against the pinned actual Core21 SRPM, spec and archive hashes.
The spec change is completely reversible; the output archive stays byte exact.
The COPR wrapper performs the source download and runs the same preparer.
The prior installed payload checks remain in the recipe. No new RPM name,
service, native tablet build, install or boot operation is introduced.

Before changing production providers, verify the signed RPM dependency and
payload delta, original Fedora library ABI, current/empty/late/unknown data and
charge errors, configuration persistence, actual DNF preview and installation,
fresh installation and signed rollback. Preserve the current provider until
the relevant physical and userspace parity gates pass. A dependency-only
preparation is not permission to report that stock UPower is installed.
