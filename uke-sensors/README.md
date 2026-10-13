# uke-sensors

Uke sensor discovery, typed evidence and native userspace integration for Xiaomi Pad 7 / POCO Pad X1 (`uke`, SM7675).

Native metadata diagnostics are available in source. The packaging scaffold is
still gated: these tools do not provide IIO sensor streams, touch input or a
calibrated desktop sensor interface.

## Metadata diagnostics

`make test-native` builds the tools and runs host wire-format and malformed-input
checks. Build for AArch64 before any authorized tablet trial; a host test is not
a device result.

- `uke-sensor-discovery accel` sends one bounded SSC SUID lookup. Supported types
  are accel, gyro, mag, ambient_light, proximity, sar, hall and sensor_temperature.
  `all` follows the first discovered endpoints with standard metadata requests;
  it prints names/types and availability, excluding IDs and calibration data.
  Each socket has a five-second deadline and packet/size limits. No sensor rate,
  updates subscription, factory mode, arbitrary message ID or calibration write.
- `uke-touch-readonly` requires the exact Uke/Novatek SPI leaf and an explicitly
  prepared upstream spidev binding. It reads fixed NT36532 identity/firmware-info
  addresses at 1 MHz. Page-selection commands change only the volatile protocol
  address; it sends no reset, firmware download, GPIO or rail command. It does
  not bind or remove drivers itself. A qualified trial must restore the binding.

On 13 October 2026, the bounded SSC protocol and six service metadata replies
were observed in the matched Fedora test session. Physical motion/light/
compass/SAR type lookups returned no SUIDs in their first events; no samples
were acquired. The DT-corrected SPI trial identifies NT36532 family bytes, but
resident firmware-info validation fails. These are bring-up results, not usable
sensor or touchscreen support. Native AArch64/QEMU checks and ASan/UBSan pass;
physical calibration and input acceptance remain open.

Protocol provenance: [Qualcomm sensinghub](https://github.com/qualcomm/sensinghub/tree/329551963bcc063f6a89f6cbca259c5b5c9c4e0b).
The trim/event register evidence is cross-checked between a retained direct-Uke
module and [MiCode's NT36532 source](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/tree/77dc8e2290f3f5f15861821564f68d30ea193449/drivers/input/touchscreen/nt36532).
Generic protocol and adjacent-controller evidence do not admit the complete
OEM firmware ABI, electrical wiring or another tablet's calibration.

## Package scope

- `uke-sensors`

Nabu reference families: `nabu-sensors`.
These are process references; Uke wiring, firmware and runtime behavior require
independent implementation and validation.

## Required validation

- Review Uke LSM6DSO, STK3BCx, SIP1328, QMC630x and SX937x sources.
- Prove SSC transport and per-profile sensor wiring.
- Calibrated measurement and IIO/desktop acceptance.

The first packaging target is Fedora Rawhide AArch64 in
[uke-linux-test COPR](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-test/).
DEB and Alpine APK targets require their own native rules later.
Unimplemented targets fail explicitly rather than producing placeholder RPMs.

Use `make validate` to check this repository's manifest contract. Native device
applications use C++; upstream C/assembly interfaces retain their original
languages. Host automation uses Bash or Make. Python does not enter tablet
payloads. Source archives belong in `referances/`, development in `src/`.
Public records exclude personal paths, unit identifiers and calibration data.

See [the roadmap](docs/ROADMAP.md), [source policy](docs/SOURCE-POLICY.md) and
[manifest](manifests/component.json). New material findings are recorded in
`docs/lessons/`; source, package, emulation and physical evidence stay distinct.
