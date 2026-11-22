# Repository layout

`components/` groups source by owner. Use `src/` for maintained code,
`packaging/` for RPM rules and support files, `tests/` for checks,
`manifests/` for versioned inputs, `references/` for source archives, and
`docs/` for public guides. Uke Control user guides live under
`components/uke-control/docs/guides/`.

`packaging/families/` defines Fedora RPM source families and compatibility
rules. Its `host/` directory contains composition scripts, `tests/` contains
family contracts and `consolidation/` holds migration inputs. The
[family responsibilities](../../packaging/families/RESPONSIBILITIES.md)
describe ownership and integration boundaries.

`packaging/ubuntu/` contains Debian-format source export recipes and
their pinned inputs. Published source packages use commit-specific paths
and checksums. The repository moves apply to the current checkout.
`packaging/kernel-security/` contains the associated kernel security inputs.

`manifests/source-imports.json` records the original component repositories
and import commits. Checkout directory names are independent of RPM and
service names. The `third-party-repos/` component retains its published
RPM name `thirparty-repos`.
