# Third-party DNF repositories for Uke

On Fedora Rawhide AArch64, install `thirparty-repos` from Uke Unstable.

```sh
sudo dnf install thirparty-repos
```

This package registers the following signed RPM repositories.

- Google Chrome Stable, Beta, Dev and Canary
- Brave Stable and Nightly
- Vivaldi Stable
- Visual Studio Code and Insiders
- Cursor
- Warp Terminal
- Mullvad VPN Stable
- 1Password CLI
- ONLYOFFICE Desktop Editors
- AnyDesk
- RPM Fusion NVIDIA Driver from Fedora Workstation, disabled by default
- OpenAI's ChatGPT desktop preview, disabled by default

Install applications in a separate DNF transaction.
