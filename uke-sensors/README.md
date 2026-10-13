# uke-sensors

Uke sensor discovery, typed evidence and native userspace integration for Xiaomi Pad 7 / POCO Pad X1 (`uke`, SM7675).

Native metadata and bounded sample diagnostics are available in source. The packaging scaffold is
still gated: these tools do not provide IIO sensor streams, touch input or a
calibrated desktop sensor interface.

## Metadata diagnostics

`make test-native` builds the tools and runs host wire-format and malformed-input
checks. Build for AArch64 before any authorized tablet trial; a host test is not
a device result.

- `uke-sensor-discovery accel` sends one bounded SSC SUID lookup. Supported types
  are accel, gyro, mag, ambient_light, ambient_light_back, ambient_light_back_strm,
  proximity, sar, hall and sensor_temperature.
  `all` follows the first discovered endpoints with standard metadata requests;
  it prints names/types and availability, excluding IDs and calibration data.
  Each socket has a five-second deadline and packet/size limits. No sensor rate,
  updates subscription, factory mode, arbitrary message ID or calibration write.
- `uke-sensor-sample accel|gyro|ambient_light|ambient_light_back|ambient_light_back_strm`
  requests one bounded data event. Streaming types request 1 Hz, subject to the
  DSP's supported minimum rate; on-change light requests have no rate payload.
  Motion requests batch for 100 ms and settle for three seconds within the
  existing five-second/64-packet limit. The client closes after the next standard
  data event; light requests close after the first event. XYZ dimensions,
  timestamps, finite floats, source identity and accuracy status are checked.
  Rear-light channels stay raw until their vendor units are verified.
- `uke-touch-readonly` requires the exact Uke/Novatek SPI leaf and an explicitly
  prepared upstream spidev binding. It reads fixed NT36532 identity, topology and firmware-info
  addresses at 1 MHz. Page-selection commands change only the volatile protocol
  address; it sends no reset, firmware download, GPIO or rail command. It does
  not bind or remove drivers itself. A qualified trial must restore the binding.

Earlier on 13 October 2026, the bounded SSC protocol and six service metadata replies
were observed on the owner's matched Fedora session. Physical motion/light/
compass/SAR type lookups returned no SUIDs in their first events; no samples
were acquired. The DT-corrected SPI trial identifies NT36532 family bytes, but
resident firmware-info validation fails. These are bring-up results, not usable
sensor or touchscreen support. Native AArch64/QEMU checks and ASan/UBSan pass;
physical calibration and input acceptance remain open.

Later copr40 RAM tests with the native registry cache, measured SoC profile and
unit DSP library return LSM6DSO acceleration/gyro XYZ data and SIP1328 rear-light
channel data. First motion samples report accuracy 0 (unreliable); rear light
reports 3, with channel units still unverified. Front light/proximity lookups
remain empty. A later settled, batched motion trial reaches status 3, with
plausible stationary gravity and near-zero gyro readings. Controlled movement,
drift and calibrated accuracy remain unqualified. All 36 existing factory-calibration group files remain unchanged
after cache initialization. These results do not provide automatic brightness,
desktop rotation, calibrated accuracy or touch input. No experimental daemon is
permanently enabled and no sensor RPM is admitted.

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
# Read-only OEM calibration

The development image mounts `PARTLABEL=persist` at `/mnt/vendor/persist` with
`ro,noload,nodev,nosuid,noexec`, no automatic fsck and a bounded optional-device
wait. Warm-boot mounting and unchanged block hashes are observed. Sensor
calibration remains private; a successful mount does not prove SEE consumed it.

The FastRPC fileservice candidate uses pinned original
[hexagonrpc](https://github.com/linux-msm/hexagonrpc/tree/da7a3742ab8d5273a281b70ee11f16fb2cb6a690)
C sources. The read-only backend triggers a SEE registry assertion on Uke.
The pinned write-capable comparison, identified in `manifests/hexagonrpc-source.json`,
is qualified with descriptor, native format and OEM `/odm` mapping patches.
Place its exact archive at `referances/hexagonrpc-write/source.tar.gz`, then run
`make test-hexagonrpc` on the host. This exercises RPC, buffer, mapped-file and
actual registry create/write/rename/read contracts without Python.
Original `persist` stays read-only; SEE writes belong in a separate runtime cache
seeded from it. Android inode labels must not be copied into that native cache.
No sensor RPM, automatic daemon activation or physical sample acceptance follows.

## Normal touch firmware laboratory probe

`touch-normal-probe.cpp` is a separate, gated C++ laboratory tool, not a shipped
driver or startup service. It admits only the pinned normal O82 CSOT image and
the measured Uke cascade controller. `manifests/touch-normal-probe-source.json`
records protocol, image and host SDK provenance; firmware redistribution is not
admitted. `make test-touch-normal TOUCH_NORMAL_FIRMWARE=PATH` uses a privately
retained image to check exact bytes, every transfer failure, factory-command
refusal, pre-boot bank readback, bounded waits and resident metadata.

An authorized RAM trial now programs and reads back all four banks byte for byte,
then reaches firmware version 17 with 60×40 channels and raw bounds 32000×21360.
The OEM configuration specifies super_resolution=10; those raw bounds are not
Linux screen pixels. Earlier post-boot result-slot CRC comparison and a generic
8192 coordinate limit were rejected assumptions. Complete pre-boot byte equality
and exact resident metadata replace those assumptions; post-boot result values
remain diagnostic. No touch input is accepted by this progress.

The reviewed coordinate-mode command is restricted to subcommand 0x15 and values
zero/one. Its bounded probe captures raw prefixes only and restores raw mode on
success or transfer failure. Factory calibration subcommands remain forbidden.
Coordinate packet decoding, IRQ/input integration and a kernel driver remain
separate physical gates. The laboratory tool uses the existing native OpenSSL
runtime and adds no Python or firmware blob to a tablet package.
