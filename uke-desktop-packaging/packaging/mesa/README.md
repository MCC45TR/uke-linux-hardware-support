# Mesa AArch64 host-feature correction

The exact Fedora `mesa-26.2.4-1.fc46` source RPM is pinned in
`manifests/native-runtime.json`. One upstream C++ conditional admits AArch64 to
LLVM host-feature detection. Disabled features must override CPU-name defaults:
the physical Uke llvmpipe crash stopped at an SVE instruction while Linux
reported no SVE. KDE applications and the original Fedora driver selection,
APIs and build flags are unchanged.

The build worker runs the exact patched feature function against LLVM's actual
feature map, checking every enabled and disabled attribute. The unpatched
AArch64 branch must fail this fixture. Signed payload, offline DNF rollback,
physical Wayland stability and camera-observed output remain separate gates.
This is a software-rendering correction; native Adreno is still unqualified.

Unavoidable upstream host tools: Meson `1.12.1-2.fc46`, Python development/runtime
`3.15.0-1.fc46`, and LLVM development libraries `23.1.2-1.fc46` are pinned in
the derived recipe. Mesa's upstream Mako `1.4.1-2.fc46`, PyYAML `6.0.3-6.fc45`
and Pycparser `3.00-4.fc45` source generators
run only in the build worker; replacing them has no practical narrow alternative.
Actual worker dependency versions stay in COPR logs. Every binary subpackage is
checked for Python files/shebangs before packaging and needs independent native
payload/dependency admission before delivery. Detailed physical evidence remains
in the private documentation journal.
