# Swap startup admission

17 November 2026. Control 2.17.0-28 retains the complete signed Control27
runtime and source recipe. Kernel, charging, root-capacity checks and saved
preferences stay in their existing owners.

The observed critical path runs `uke-control boot` (including full swap
application) and then `uke-control swap apply` again in a local readiness
unit. On the accepted tablet this repeats approximately 2.3 seconds of
read-only storage admission. Root capacity is separately checked before both
operations and must remain so.

`swap apply` still performs the entire original admission every time. It
invalidates any old startup receipt first, records the current boot, selected
policy and block inventory before admission, and publishes a private receipt
in RAM only when admission succeeded and the inventory remained unchanged.
There is no periodic refresh, disk cache or background process. Failure to
cache success leaves full admission available.

The graphical-session barrier invokes `swap ready`. It reuses that success
only with the same boot, inventory and policy, a root-owned 0700 parent and
0600 single-link regular receipt, trusted original helper, active block swap
and a matching owned `linux_swap` mapper. Capacity must agree with active swap
minus its 4-KiB header; UUID, device number, swap signature and label must match.
A nonzero device-mapper event count, changed descriptor identity, other managed
active swap files or any missing/unknown observation goes through full admission.
New block devices invalidate the inventory. Privileged raw metadata changes
without kernel notification are outside a startup receipt's contract; explicit
`swap apply` never reuses the receipt.

The original partition-first selector is byte identical: the physical UFS
source, three LP slots, mapping ownership/table/size, uniqueness, label and
complete swap header are still validated by the full operation. A valid unique
`linux_swap` takes precedence over automatic managed files. No partition is
formatted. Existing managed files are retired only with the existing memory
and ownership checks. Physical partitions and file fallback take the full path.

The existing `uke-swap-ready.service` name is adopted inside Control. A small
service drop-in updates only its command and journal output, so local
root-capacity dependencies remain intact. The original Control boot task,
startup receipt and standalone `swap apply` semantics remain. User sessions
require this barrier. No service is disabled and no timeout is shortened.

COPR tests execute the production admission/ready code with private files and
injected block metadata. They cover first admission, same-boot reuse, manual
requests, boot/policy/inventory changes, mapper events/signatures/capacity,
file ownership/modes/links, active managed files, descriptor substitution,
inventory changes during verification and refusal propagation in four native
compiler/sanitizer modes. Original Control25/26/27 tests remain in the recipe.
Software fixtures do not establish a boot speed improvement; signed delivery
and comparison of actual boots remain separate acceptance gates.

Hibernate is a separate capability. The current swap exceeds RAM, but the
accepted kernel refuses hibernation under integrity lockdown. Do not turn off
lockdown or claim resume support from a swap-size check. A future conditional
policy must also require actual kernel and resume capability; secure resume
and physical hibernation acceptance remain open.
