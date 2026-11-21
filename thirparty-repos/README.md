# Third-party DNF repositories for Uke

On Fedora Rawhide AArch64, install `thirparty-repos` from Uke Unstable.
It installs DNF repository definitions. Use a separate DNF transaction
to install the applications you want.

```sh
sudo dnf install thirparty-repos
sudo dnf install code code-insiders brave-browser mullvad-vpn 1password
sudo dnf install solidcraft orca-slicer prusa-slicer
sudo dnf install google-chrome-stable google-chrome-beta google-chrome-unstable google-chrome-canary
```

- VS Code and Insiders, Brave Stable, Mullvad VPN Stable and 1Password use
  their publishers' signed RPMs. The 1Password repository metadata is
  signed as well.
- All four Chrome channels use Google's signed ARM64 RPMs. Google calls
  its Dev package `google-chrome-unstable`. Fedora Workstation provides
  Chrome Stable's default file, and Uke provides the other three.
- Fedora Workstation's curated third-party repositories follow Fedora's
  opt-in setting. PrusaSlicer is available from Fedora Rawhide itself.
- SolidCraft uses the published ARM64 RPM payload and OrcaSlicer uses the
  published ARM64 AppImage payload. COPR signs both Uke RPMs.
  [SolidCraft v0.45.0 release](https://github.com/tangency-labs/solidcraft/releases/tag/v0.45.0)
  and [OrcaSlicer v2.5.2 release and source](https://github.com/OrcaSlicer/OrcaSlicer/releases/tag/v2.5.2).

Check the upstream digest and architecture when importing newer CAD or
slicer releases. Repository definitions and application RPMs are
updated separately.
