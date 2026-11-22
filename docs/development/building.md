# Build and source validation

Run the source checks from the repository root.

```sh
make validate
make -C packaging/families check
```

The [source workflow](../../.github/workflows/source-contracts.yml) defines
the Fedora Rawhide host tools and runs source checks. The family checks also
verify pinned Fedora and COPR inputs. Native package compilation and signed
RPM delivery run through COPR.

The [COPR entry point](../../.copr/Makefile) selects a reviewed source
subdirectory. Current component paths start with `components/`; family
paths start with `packaging/families/`. Existing COPR definitions tied to
an older commit resolve paths at that commit.

The [Ubuntu source guide](../../packaging/ubuntu/README.md) describes
Debian-format source export. The [Control component](../../components/uke-control/)
owns its terminal, device and shared-client source interfaces.
