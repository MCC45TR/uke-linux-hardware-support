# Third-party DNF repositories for Uke

Install `thirparty-repos` from Uke Unstable on Fedora Rawhide AArch64. It
selects both repository configuration subpackages. In a separate DNF
transaction, install the browser channels and CAD application you want.

```sh
sudo dnf install thirparty-repos
sudo dnf install google-chrome-stable google-chrome-beta google-chrome-unstable google-chrome-canary solidcraft
```

All four Chrome channels use Google's signed ARM64 RPM repository. Google
uses the RPM name `google-chrome-unstable` for the Dev channel. The Chrome
defaults keep clean installations on one repository. RPM preserves
existing administrator customization.

SolidCraft's upstream project publishes RPM downloads. Uke serves the
original ARM64 RPM payload in the SolidCraft directory of Uke Unstable
and signs it with the Uke COPR key. The initial import is SolidCraft
`0.45.0-1.aarch64` from upstream release `v0.45.0`, commit
`e8ce12ef78fe76281a04b7242a9ce059ff7d5067`. The upstream RPM's SHA-256
is `6573041f777e973877b0eb01c0b917e2278e36887a50905d49cf5cc685d7f751`.
[SolidCraft release](https://github.com/tangency-labs/solidcraft/releases/tag/v0.45.0)

The SolidCraft directory accepts imports of published upstream ARM64 RPMs.
Check the upstream checksum and package identity before importing a newer
release. Repository RPM updates and application RPM updates are separate.
