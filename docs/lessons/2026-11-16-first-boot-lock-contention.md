# First-boot lock contention and sandbox state creation

- Date: 16 November 2026
- ID: UKE-FIRST-BOOT-LOCK-20261116
- Environment: original Fedora Plasma full singleboot EXT4 installation,
  Control25, cumulative 7.3.4-4 on POCO Pad X1; separate x86_64 development PC.
- Evidence: the first physical boot enabled the grown logical swap. The later
  swap-ready service then failed with `Another uke-control operation is running`.
  Its required dependency prevented both Plasma Login and Plasma Setup from
  starting. Kernel-approval failures followed the absent display manager.
  The next physical boot reached normal Wayland and native health accepted it.
- Additional evidence: Chrony's first clock-restore hook could not create the
  profile directory; the installation classifier created it shortly afterward,
  and Chrony's retry succeeded. Its strict sandbox allows the existing floor
  path but did not declare StateDirectory. This ordering is consistent with
  creation failing before the allowed path exists.
- Change: boot swap application and temporary setup USB wait on the existing
  flock; manual CDC retains its nonblocking default. Original trusted lock and
  storage admission checks remain. Declare the existing floor directory as
  systemd-managed state before Chrony's sandbox starts, with mode0700.
- Source validation: four complete source inverses and the complete signed25
  recipe inverse pass; all81 parent inputs and three RPM names are retained.
  Native GCC/Clang plain/sanitizer contention, release, cancellation, refusal,
  setup-policy and logical-swap checks are prepared for COPR, not run on the PC.
- Limit: the changed package is not yet built, delivered or cold-boot accepted.
  Current normal-session success does not qualify the corrected first boot.
- Next: verify actual COPR inputs and signed outputs, deliver through DNF without
  changing saved policy, and qualify the next fresh first-setup boot separately.

COPR11336297 stops in source lint before producing an SRPM. ShellCheck flags
the manifest output inside its traversed directory and an unquoted compiler
option containing a comma. Generate the manifest outside the traversed tree,
then install it, and quote the single compiler option. Keep lint and every
native/runtime gate enabled. No Control26 package was published by this trial.

COPR11336307 passes source lint, then rejects the parent SRPM digest. The local
source-worker copy precedes publication signing. The published parent has two
valid signatures under2204A661EA630A5303500085538797E72A3E4ECA; all81 source
files and the complete spec byte-match the source-worker copy. Pin the published
signed wrapper digest9257a5ec..., retaining the source-worker digest separately.
The signature, spec, per-input and whole-recipe guards are unchanged. This trial
also produced no SRPM or runtime package.
