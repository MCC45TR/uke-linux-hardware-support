# Independent Rawhide packages

29 October 2026: `mcc45tr/uke-linux-unstable` targets only
`fedora-rawhide-aarch64`. Each component has its own source package and build.
Conventional upstream subpackages, such as a library and its development files,
remain in that component's source build. The nine umbrella source packages are
not submitted to this repository.

30 October 2026: the [source receipt](unstable-source-receipt-20261030.json)
records 33 verified producer definitions and their installed RPM provenance.
Thirteen stale defaults were restored to the exact successful COPR recipes.
The kernel's later source guard requires a fresh release before another build;
conventional optional components remain separately installable.

`host/standalone.sh NAME OUTPUT` runs in the COPR source worker. It uses the
owning component's recipe or its existing exact-pinned public source archive.
The shared specification converter retains package names, capabilities,
licenses, build checks, file ownership and applicable service migration hooks.
No binary RPM is repackaged. Local touch calibration and the locally extracted
VPU firmware are excluded from public source inputs and channel replacement.

The core selection depends directly on runtime packages. Its standalone build
provides bounded compatibility aliases and obsoletes the old umbrella selection
RPMs, so an installed system can migrate in one dependency-resolved transaction.
Those aliases add no source-family RPM to the new repository.

All compilation and RPM checks run in COPR. Its custom source method requires
an unpacked specification and sources: extract the generated source RPM into
`COPR_RESULTDIR` before returning. Declare source-worker dependencies in the
build request; this worker is unprivileged and cannot install them itself.

GitHub validates source and packaging metadata without compiling native tests.
Native package checks remain in the owning COPR RPM build. The aggregate native
runner and root-only signed-repository fixture remain separate checks; a source
workflow pass does not establish that either was executed.

A successful source build or signed RPM does not establish tablet hardware,
kernel activation, reboot or rollback acceptance. Keep `automatic=0` while
installing packages during an active tablet session.
