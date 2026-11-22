# Research references

The 14 October 2026 source inventory is
[`manifests/research-donors-20261014.json`](../manifests/research-donors-20261014.json).
It maps the local `donors/` checkouts to exact commits, purpose and verification
limits. Large source trees remain local and are excluded from Git.

Before changing hardware support, compare Uke OEM evidence, the relevant
same-SoC or shared-IP donor and current upstream code. Follow the workspace's
`uke-linux-docs/docs/DONOR-POLICY.md` and record the useful difference, exact
source files, attribution and Uke compatibility limits in its lesson journal.

This inventory does not change a build lock or prove source dependency closure,
redistribution rights or tablet support. Do not run donor scripts or develop
inside these checkouts. Uninitialized submodules and unaudited LFS objects
remain outside the verified Git checkout.
