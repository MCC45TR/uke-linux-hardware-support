# Uke Linux Hardware Support

Device integration, platform services and Fedora packages for **Xiaomi Pad 7**
and **POCO Pad X1** (`uke`, Qualcomm SM7675).

[Main project](https://github.com/MCC45TR/uke-linux) ·
[Kernel](https://github.com/MCC45TR/senemos-uke-kernel-mainline) ·
[Fedora image builder](https://github.com/MCC45TR/uke-fedora-builder) ·
[Development COPR](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-test/)

## Repository layout

```text
uke-linux-hardware-support/
├── .copr/Makefile
├── .github/workflows/
├── components/
│   ├── uke-boot/
│   ├── uke-control/
│   ├── uke-control-gtk/
│   ├── uke-control-qt/
│   ├── uke-platform-runtime/
│   ├── uke-sensors/
│   ├── xiaomi-uke-firmware/
│   ├── uke-desktop-integration/
│   ├── uke-orangefox-packaging/
│   └── third-party-repos/
├── docs/
│   ├── architecture/
│   └── development/
├── manifests/source-imports.json
├── packaging/
│   ├── families/
│   ├── kernel-security/
│   └── ubuntu/
└── references/
```

| Area | Source owners |
| --- | --- |
| Boot, updates and recovery | [Boot](components/uke-boot/), [OrangeFox](components/uke-orangefox-packaging/), [image tooling](components/uke-image-tools-packaging/) |
| Device controls | [Control](components/uke-control/), [GTK](components/uke-control-gtk/), [Qt](components/uke-control-qt/) |
| Runtime and sensors | [Platform runtime](components/uke-platform-runtime/), [sensors](components/uke-sensors/), [NFC and infrared](components/uke-nfc-ir/) |
| Desktop | [Integration](components/uke-desktop-integration/), [packaging](components/uke-desktop-packaging/), [desktop selection](components/uke-desktop-metas/), [Plasma settings](components/plasma-uke-kcm/) |
| Firmware and provenance | [Xiaomi firmware](components/xiaomi-uke-firmware/), [hardware provenance](components/uke-hardware-provenance/) |
| Optional integration | [Video](components/uke-video/), [camera](components/uke-camera/), [Waydroid](components/uke-waydroid/), [third-party repositories](components/third-party-repos/) |

[Source ownership](docs/architecture/repository-layout.md) ·
[Build and source validation](docs/development/building.md) ·
[Package responsibilities](packaging/families/RESPONSIBILITIES.md) ·
[Historical import identities](manifests/source-imports.json)

Public guides describe interfaces, package ownership and supported workflows.
Hardware acceptance is tracked in the [device support table](https://github.com/MCC45TR/uke-linux#hardware-support).

Project-owned code is covered by the [MIT License](LICENSE) unless noted
otherwise. Imported components and firmware retain their own terms.
