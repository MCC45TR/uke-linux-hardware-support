# Fedora policy compatibility candidate

3 November 2026. This COPR-only producer retains Fedora's installed
`selinux-policy-45.23-1.fc46` source package and advances its Release to `1.1`.
The original source archive, contributed modules, policy capabilities, unknown
permission setting, package names and lifecycle scripts remain unchanged.
No kernel policy is loaded during the build or its checks.

The patch defines the kernel's newer system, BPF and io_uring permissions and
the memfd_file class. It adapts the upstream reference-policy firmware interface
for the kernel and the existing module-loader attribute. The module-loader rule
retains the `secure_mode_insmod` condition. Only callers of Fedora's existing
io_uring creation interface gain the new setup permission. Inherited descriptor
access alone does not grant setup. BPF-token and memfd policy capabilities
remain disabled, retaining Fedora's existing compatibility models.

Attribution and primary inputs:

- [Fedora source](https://github.com/fedora-selinux/selinux-policy/tree/8a22f4474f77c7cb32cd6d18e3da75eaef8696de), GPL-2.0-or-later.
- Christian Göttsche's [class and permission definitions](https://github.com/SELinuxProject/refpolicy/commit/d4084b9facd767188d64e658c35ee6a0e8261a16).
- Guido Trentalancia's [firmware permission](https://github.com/SELinuxProject/refpolicy/commit/bd53d7520a04e64635202c15b65bed12cc44b59e), [remaining system permissions](https://github.com/SELinuxProject/refpolicy/commit/7a0779dee785f905d84814004b103059257ceb74) and [firmware interface](https://github.com/SELinuxProject/refpolicy/commit/38d074bcad0783d3624ecc22b0adfbae9c8e6566).

The source worker verifies the exact original SRPM hash. Koji's build output is
unsigned; the public targeted-policy baseline is checked against payload and
policy digests from Fedora's signed RPM header. No live policy, local module or
device capture is uploaded. The original Fedora Python build dependencies stay
on the build worker; this change introduces no project-owned Python.

The proposed check queries all three built policy profiles, compares capabilities
and permissive types with the original targeted policy, checks seven positive
and negative access cases, and compares twelve existing permission queries.
These are build and policy-query checks. Installation, live AVC monitoring,
firmware reload and boot acceptance require separate device evidence. Definitions
alone do not establish complete policy for every newer kernel operation.
