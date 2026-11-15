# UKE-USB-API-OUTPUT-20261116: budget the consolidated capability list

Date: 16 November 2026. Environment: Fedora Rawhide on POCO Pad X1,
kernel 7.3.3-28-unstable, signed Control 2.17.0-20; separate development PC.

## Observation

The native manual CDC request refused the matched API before touching USB.
A bounded exec trace identified the original RPM query and the collector
killing its child. The installed package provides both USB API capabilities
at version 2, but its complete formatted capability list is 4,225 bytes.
The three USB validation calls allowed only 4,096 bytes. Consolidating other
features into Control crossed that limit. The parser and query escapes were
correct; the earlier escaping hypothesis was rejected by actual argv and
tab/newline observations.

## Change and evidence

Control 21 derives the signed successful 20 source and keeps all 68 inputs,
including RGB 20 and Motion 19. A complete recipe inverse preserves earlier
build phases, checks, hooks and ownership. Only the three query output budgets
grow to 65,536 bytes; the five-second bound, exact API version, duplicate
rejection, board checks, trusted helper and USB ownership checks remain.

The additional COPR test extracts the actual bounded collector and uses the
original RPM executable against the signed parent runtime. It must reproduce
the old truncation, accept both actual API2 entries with the new budget, and
reject wrong versions, duplicates and incomplete output. It also tests exact
limit versus one-byte overflow under GCC/Clang and their sanitizers. No C++
compilation runs on the PC or tablet.

## Limits and next validation

Source changes and the PC recipe inverse are separate from native build,
signed DNF, actual CDC enumeration and first-setup lifecycle acceptance.
This change does not enable a root shell or change the saved CDC preference.
Original Plasma setup commissioning and marker-driven cleanup remain open.
