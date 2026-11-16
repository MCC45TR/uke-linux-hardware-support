# Logical swap admission and the installed CDC reboot owner

- Date: 16 November 2026
- ID: UKE-LOGICAL-SWAP-CDC-20261116
- Environment: signed Control24 on POCO Pad X1, cumulative 7.3.4-4/A;
  source preparation on a separate x86_64 PC.
- Evidence: LP `system_a` is a DM mapping, so the former mandatory sysfs
  `partition` marker rejects it. Treating its label as sufficient would admit
  foreign devices. The setup console also called `/usr/bin/uke-reboot`, which
  the consolidated package explicitly removes. Runtime inspection confirms
  the missing executable and a bound kernel reboot-mode backend.
- Change: demand-map only the exact ready seed4 allocation; independently
  verify owned DM state before existing swap activation and refuse failures
  before file fallback. Use original installed systemctl for the fixed CDC
  bootloader request. Keep original Fedora binaries and all prior guards.
- Validation: PC text/preimage/whole-inverse checks are distinct from pending
  COPR native checks, owned-loop negative controls, DNF and physical tests.
- Limit: this does not enable encrypted swap, authenticated hibernate, rescue
  boot/control or the requested additional terminal reboot targets.
- Next: qualify the actual prepared consumer and packaged files; deliver by
  signed DNF, preserve saved preferences and test first-setup USB cold boot.

COPR11334341 passes the four new swap modes, four changed CDC modes, retained
CDC source proof and production build, then fails the new packaged-script byte
comparison. Fedora brp-mangle-shebangs converts `env bash` to `/usr/bin/bash`.
The check now admits precisely that first-line normalization and still compares
every other byte. No runtime, source or native gate is removed; release25 remains
unused because this attempt failed before publication.
