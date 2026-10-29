# Independent unstable packages

29 October 2026. Source preparation targets `mcc45tr/uke-linux-unstable` and
exactly `fedora-rawhide-aarch64`. Thirty component recipes have independent
COPR source entries. Native builds and tablet channel migration remain pending.

The factory reuses the pinned source inputs, package naming rules, checks and
scriptlets from the existing recipes. It emits one source RPM per component;
the kernel keeps its matched core, modules, DTB and boot bundle. Development and
runtime subpackages from the same upstream component retain their exact version
dependencies. Existing stable and test source factories remain available.

The nine empty family selection RPMs are retired with explicit
Provides/Obsoletes declarations. The actual Core metadata package retains their
installation dependencies. Libraries do not acquire dependencies on unrelated
members of their former build group. Package releases are independent and use
the `.unstable1` suffix before the Fedora dist tag. Original sources, patches,
licenses and source hashes remain in each source archive.

[`packages.json`](packages.json) lists the public source names, original recipe
inputs, independent releases and legacy selection replacements. The `group`
field locates an existing recipe; it does not register a family in COPR.
Source preparation and every native build/check run in COPR. The source worker
runs the [recipe fixture](tests/recipes.sh) before preparing a real component.
The local [manifest check](tests/manifest.sh) performs no compilation.

Publish and audit the signed component packages before switching a tablet with
`uke-control channel set unstable --dry-run`, followed by the same command with
`--yes`. The updated manager requires the new project's reviewed signing-key
fingerprint. An unregistered key, missing component, unmatched kernel tuple or
dependency failure blocks the transition. Device-private firmware stays local.
Channel selection does not request a reboot; kernel activation retains the
existing updater gate and requires a separate device validation window.
