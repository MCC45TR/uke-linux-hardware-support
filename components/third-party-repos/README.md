# Third-party DNF repositories for Uke

On Fedora Rawhide AArch64, install `thirparty-repos` from Uke Unstable.
It installs DNF repository definitions. Use a separate DNF transaction
to install the applications you want.

```sh
sudo dnf install thirparty-repos
sudo dnf install code code-insiders brave-browser mullvad-vpn 1password-cli
sudo dnf install solidcraft orca-slicer prusa-slicer
sudo dnf install onlyoffice-desktopeditors rustdesk
sudo dnf install google-chrome-stable google-chrome-beta google-chrome-unstable google-chrome-canary
```

- VS Code and Insiders, Brave Stable, Mullvad VPN Stable, ONLYOFFICE Desktop
  Editors and 1Password CLI use
  their publishers' signed RPMs. The 1Password repository metadata is
  signed as well.
  The 1Password desktop application is available from the vendor as an
  [ARM64 archive](https://support.1password.com/install-linux/#arm-or-other-distributions-tar-gz).
- All four Chrome channels use Google's signed ARM64 RPMs. Google calls
  its Dev package `google-chrome-unstable`. Fedora Workstation provides
  Chrome Stable's default file, and Uke provides the other three.
- Fedora Workstation's curated third-party repositories follow Fedora's
  opt-in setting. PrusaSlicer is available from Fedora Rawhide itself.
- SolidCraft uses the published ARM64 RPM payload and OrcaSlicer uses the
  published ARM64 AppImage payload. COPR signs both Uke RPMs.
  [SolidCraft v0.45.0 release](https://github.com/tangency-labs/solidcraft/releases/tag/v0.45.0)
  and [OrcaSlicer v2.4.2 release and source](https://github.com/OrcaSlicer/OrcaSlicer/releases/tag/v2.4.2).
  The ARM64 RPM uses Fedora libraries and selects the latest stable AppImage
  release whose launcher and slicer do not require an embedded Python runtime.
- RustDesk uses the official ARM64 release payload in a COPR-signed RPM.
  The background service follows Fedora's systemd presets. Fedora disables
  it on a fresh installation. Wayland screen capture uses the desktop portal.
  [RustDesk v1.5.3 release](https://github.com/rustdesk/rustdesk/releases/tag/1.5.3)

Check the upstream digest and architecture when importing new application
releases. Repository definitions and application RPMs are
updated separately.
