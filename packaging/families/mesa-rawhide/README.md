# Uke Mesa packaging

The [Mesa workflow](../../../.github/workflows/mesa-rawhide.yml) follows
the Mesa source corresponding to Fedora Rawhide's published AArch64
driver packages. It verifies the Fedora SRPM
signature, reuses the Uke recipe and regression checks, and submits an SRPM
to a side repository in Uke Unstable COPR. After COPR signs the compiled
RPMs, the workflow verifies those inputs and submits the combined runtime
and matching development packages to the regular Uke Unstable repository.
The runtime keeps the installed `uke-mesa` package name and the existing
LLVM 22 SPIR-V builtins.

A new upstream version starts at RPM Release 1. New Fedora sources with
the same upstream version use the next unused release. The tracked source
and both COPR build IDs are recorded after submission. An active build
defers the next step to the following scheduled check.

The Freedreno and Turnip patches in this repository complement the pinned
Uke packaging revision in `support-commit`. Incompatible source changes
surface as a failed build so the reviewed graphics deltas can be updated.
The workflow can also be started manually from the default branch. Failed
or canceled builds stay visible in the workflow. The manual retry option
allocates a new release for a failed source once the recipe is ready.
