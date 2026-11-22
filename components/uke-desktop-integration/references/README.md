# Source archives

Keep verified reference archives here, ignored except for this policy file.
Never modify or execute donor scripts as a build shortcut.

Graphics and codec research from 18 October 2026 is pinned in
[`manifests/graphics-research-20261018.json`](../manifests/graphics-research-20261018.json).
It records exact Chromium source snapshots, shared component-owned OEM/kernel
checkouts and local stateful VA-API research clones. These are research inputs;
no donor script is executed and no kernel build lock is advanced.

The 24 October glmark2 comparison uses package revision
`cebbb63edfba502905470c904f8e6f1c6ce28ba9` and upstream revision
`22c527cb0556f3a1ac4445aaa52cc532760928d5` from
<https://github.com/glmark2/glmark2>. Retained source headers preserve their
license and attribution. These sources were inspected, not executed or shipped.
The canonical record is `docs/lessons/2026-10-24-UKE-GRAPHICS-VALIDATION.md`
in the workspace knowledge repository.

The [24 October source inventory](../manifests/graphics-research-20261024.json)
adds the exact installed Chromium 154 revision, Khronos GLES CTS 3.2.14.1 and
the independently pinned Vulkan CTS 1.4.6.2 with its own shader dependencies.
CTS dependency archives follow its upstream fetch manifest; acquisition uses
Bash/curl/tar, without executing donor fetch or CI scripts. Audited upstream
SPIRV-Tools generators are host-only build inputs, as documented in
UKE-GPU-L019 and UKE-GPU-L023. Large immutable archives are stored on the storage volume with
links here; build scratch remains on Linux. Source acquisition and a configured
cross build do not establish a tablet CTS result.
