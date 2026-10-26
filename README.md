# Uke Linux hardware support

Native platform services, original Fedora desktop integration and RPM source families for Xiaomi Pad 7 and POCO Pad X1 (`uke`, SM7675).

**Status updated 26 October 2026.**

[Hardware support](https://github.com/MCC45TR/uke-linux#hardware-support) · [Test COPR](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-test/) · [Source families](packaging/families/README.md) · [Runtime responsibilities](packaging/families/RESPONSIBILITIES.md)

## Current device results

Testing uses POCO Pad X1, 8 GB / 512 GB, Fedora Rawhide AArch64 and original Fedora KDE. The latest boot record is Linux `7.3.0-15`; earlier physical sensor, Bluetooth, display and input results retain their original kernel/date scope.

| Function | Observed result | Limits |
|---|---|---|
| Boot/package integration | Signed coherent package installation, guarded boot readback and normal development boots | Complete installer/update interruption and variant coverage remain open. |
| Original KDE and native Control | Desktop sessions, standard interfaces and installed management/reporting operate | Expanded Control 2.3 source is separate from the recorded installed 1.14 baseline. |
| KDE Connect | `uke-kde-plasma-meta-1.0.0-10` installed from [COPR 11202438](https://copr.fedorainfracloud.org/coprs/build/11202438/); POCO F5 discovered | Pairing, sharing and post-install reboot were not tested; laptop Wi-Fi route remains separate. |
| Sensors | Four-pose rotation, dual ambient-light response, near/far proximity and cover switch events recorded | Absolute compass/temperature/color accuracy, SAR and lifecycle remain incomplete. |
| Bluetooth | Tested mouse and Space Q45 stereo/LDAC playback | HFP/microphone, broader devices and suspend. |
| Juo dock | USB hubs/mouse and working HDMI on Linux 7.3, confirmed on 26 October | DDC/CI, Ethernet traffic, reconnection and combined-load reliability remain open. |
| DisplayPort audio | ALSA card, ELD/DMA and monitor-name integration | Audible playback is unverified. Internal speakers/microphones remain unqualified. |
| Video | Default VA initialization and runtime capability enumeration in the tested development backend | Actual codec frames, browser acceleration, buffer import and sandbox remain unqualified. |
| Charging | Telemetry, conservative admission and software refusal tests | Electrical protection, cutoff/restart/restore, protocol negotiation and daily-use lifecycle remain unqualified. |
| Torch/RGB | Native source and bounded host fixtures | Actual light output and measured thermal/electrical behavior are not accepted. |

The [platform table](https://github.com/MCC45TR/uke-linux#hardware-support) is the complete public summary. Source/package success does not establish physical support, and these development results do not validate a downloadable root image.

## Desktop policy

KDE applications remain the distribution's originals. The Plasma selection includes normal Fedora autostart and a packaged firewalld policy for KDE Connect from private IPv4 and local IPv6 addresses. Pairing and administrator/user overrides remain authoritative.

Chromium is a removable default recommendation. Removing it does not require removal of the Plasma selection. No KDE application is cloned, renamed or rebuilt as a Uke variant.

Unmodified Fedora desktop packages may retain their original distribution Python dependencies. Project device helpers remain native code or shell; this exception does not add project Python to services, recovery, initramfs or installers.

## Source families

| Family | Scope |
|---|---|
| `uke-core` | Native Control and optional diagnostics |
| `uke-hw` | Boot/USB/Wi-Fi integration, installation identity, firmware and matched touch runtime |
| `uke-power` | Power observation, original desktop power integration and guarded control |
| `uke-sensors` | SSC, SensorProxy and bounded sensor support |
| `uke-additional` | Reviewed distribution library/graphics source recipes |
| `uke-extras` | Optional Plymouth theme, original decoration and recovery data |
| `uke-metas` | Separate console and original Fedora Plasma selections |
| `uke-camera` | Optional original camera application selection |
| `uke-sound` | Original ALSA/PipeWire/WirePlumber selection |

The [family map](packaging/families/families.json) and [source policy](packaging/families/README.md) define the exact build cohort. Installing an application or a metadata package does not qualify the underlying device function. Device-local touch references and calibration remain separate private inputs.

## Published development work

| Workstream | Source branch | Validation boundary |
|---|---|---|
| Graphics/video and native probes | [Native VA backend build](https://copr.fedorainfracloud.org/coprs/build/11206385/) | Selected GL pixels and capability queries are distinct from codecs, sandbox and API conformance; pending source publication retains its own scope. |
| Control NFC/battery and charging identity/PD cache | [Charging completion](https://github.com/MCC45TR/uke-linux-hardware-support/tree/feat/charging-completion-20261026) | Source/host/SRPM checks; no installed-device or electrical acceptance. |
| Native identity/diagnostic files | [Published source checkpoint](https://github.com/MCC45TR/uke-linux-hardware-support/tree/publish/source-snapshot-20261026) | Identity fixture and Vulkan/GLES host builds pass. OpenCL probe compilation lacks host headers; inactive identity helper and opt-in probes are not installed-device results. |

New work stays on its development branch until its coherent package and device gates are satisfied. This update performs no COPR submission, repository transition or tablet transaction.

## Validation and contributions

Read [AGENTS.md](AGENTS.md), the owning component's instructions and the source-family responsibility map. Run the relevant component checks, `make -C packaging/families check` and publication privacy checks before delivery. Inspect generated RPM identities, dependencies, scriptlets, signatures and rollback artifacts separately from hardware tests.

Keep source attribution and human contributor credit. Public reports should name the date, device, software, observed behavior and remaining work. Raw logs, firmware identities tied to a unit, keys and calibration remain private.

## License

Original project material uses the [MIT License](LICENSE). Imported components, source patches and distribution recipes retain their original licenses and attribution.
