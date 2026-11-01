# APT/dpkg backend implementation status

Recorded 1 November 2026. `uke-control/src/apt-policy.hpp` implements bounded,
read-only policy parsing and preliminary transaction exclusions. The Fedora
libdnf5 backend remains the active backend. No Debian Control package or APT
capability is advertised yet.

The managed Deb822 source parser accepts one enabled Tenacious ARM64 unstable
PPA with an explicit archive keyring path. It rejects duplicate fields,
additional repositories in the managed file, unsafe options, mixed suites or
architectures and enabled stable. It supports disabled entries and folded
fields. It intentionally accepts only a strict subset of
[APT's source format](https://manpages.debian.org/trixie/apt/sources.list.5.en.html).
Checking a keyring pathname is not signature verification. No keyring is
installed by this module, and the user's source-upload key is separate from
Launchpad's archive signing key.

The inventory reader accepts six tab-separated fields from the documented
[dpkg-query interface](https://manpages.debian.org/trixie/dpkg/dpkg-query.1.en.html):
`${Package}`, `${Architecture}`, `${Version}`, `${db:Status-Want}`,
`${db:Status-Eflag}`, `${db:Status-Status}`. The caller must explicitly request
this format with a newline per package; human `dpkg -l` output is unsupported.
Interrupted configuration, pending triggers, reinst-required states, duplicate
identities and foreign architectures fail closed. Preliminary exclusions block
held package changes, removals, downgrades, boot/firmware/recovery/private
payloads and duplicate plan entries. No exclusion means only that these early
checks passed; it never authorizes installation.

## Remaining integration work

1. Obtain the full system APT configuration through libapt-pkg, including
   legacy `.list` sources and overrides. The parser above handles only the
   designated managed source file; it does not establish global uniqueness.
2. Load authenticated Release/Packages metadata with the actual Launchpad
   archive signing fingerprint, freshness checks and exact candidate origins.
   Inspect selected versions, holds, Essential/Protected packages, dependency
   closure and package hashes through the solver, not scraped terminal output.
3. Bind the reviewed plan and metadata hashes to a protected operation journal,
   use the existing maintenance lock plus APT/dpkg locks, recheck immediately
   before mutation, and detect interrupted dpkg states without automatic replay.
4. Port boot finalization to the actual Ubuntu kernel/DT/modules/initramfs
   cohort. Validate extracted payloads, exact ABI and device compatibility,
   preserve the known recovery route, and select only after every stage succeeds.
5. Implement channel/status/update commands behind explicit backend selection;
   preserve Fedora behavior. Port Control's remaining RPM capability queries
   and SELinux/dracut integration before publishing its Debian recipe.

`tests/apt-policy.sh` compiles only the new project-owned fixture test with the
official C++ compiler. It performs no package command, network operation,
privileged access, source compilation from upstream, or system mutation.
