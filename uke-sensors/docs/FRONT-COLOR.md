# Raw front-color endpoint

18 October 2026. This is an isolated native C++ source/package candidate for
Fedora AArch64 on Uke / SM7675. It has not been built, installed or tested in this
workstream. A separate bounded tablet trial received one raw `cct_front` value
through the exact Uke HAL's standard event 1025 after an empty on-change request
514. That observation does not establish units, spectral calibration or
persistent operation.

The optional `uke-front-color` package publishes one local, read-only snapshot:

```text
/run/uke-sensors/front-color.json
```

It does not replace original SensorProxy, which remains the ALS/rotation
interface. It creates no IIO/sysfs device, lux conversion, kelvin value, color
management policy or desktop application variant. No Python, firmware,
calibration bytes, SUIDs or other device identifiers are included.

## Activation and lifecycle

The service is separately packaged and disabled by its install preset. Package
installation and metadata/status readers do not activate continuous collection.
After a separately authorized package/firmware and SELinux check, activation is
explicit:

```sh
sudo systemctl enable --now uke-front-color.service
```

`uke-ssc-sensors.service` readiness precedes the publisher. The publisher binds
to that service's lifetime; its opt-in dependency is restored on a resource
restart. It also checks the Uke / SM7675 compatible strings itself. Exactly one
`cct_front` SUID must advertise the expected datatype, available and physical
flags, and on-change stream type. Only then does this explicitly started
publisher send one empty request 514 on its own persistent non-wakeup QRTR
connection. It does not change calibration, registry, sensor rate or persist.

Normal stop closes the connection and removes the snapshot. Transport, SSC
server removal/replacement, malformed data or metadata failure invalidate the
sample. The publisher removes a previously live snapshot immediately and writes
an empty error snapshot within the one-update-per-second limit. It then waits
without retrying or restarting a failed DSP. A resource/service restart is
required to retry; there is no elapsed-time startup delay or restart loop.
Power, restart and suspend/resume behavior require separate tablet qualification.

Disable collection with:

```sh
sudo systemctl disable --now uke-front-color.service
```

## JSON version 1

The schema string is `org.uke.sensors.front-color.v1`. The snapshot is bounded to
4096 bytes and replaced atomically no more than once per second. The runtime
directory is owned by root and is mode 0755; the regular snapshot is mode 0644.
The publisher checks directory traversal permissions as well as ownership.
Readers can inspect raw color values without privileges. This public directory
is independent of the private mode-0700 `/run/senemos-uke` USB gadget state;
enabling CDC must not make the sensor endpoint inaccessible.

| Field | Type and meaning |
|---|---|
| `schema`, `version`, `data_type` | Fixed schema string, integer 1, and `cct_front` |
| `connected` | The current metadata-qualified connection acknowledged request 514 |
| `has_sample` | A validated raw vector is retained from this connection |
| `fresh` | Connected sample received within `freshness_window_ns` of publication |
| `available` | Connected, fresh sample with wire status 1–3 and no error |
| `error` | Empty string, `starting`, `awaiting_sample`, `metadata_unavailable`, `transport_unavailable` or `invalid_sample` |
| `values` | Raw finite float array, 1–16 entries when a sample exists; otherwise empty |
| `status` | Actual SSC wire status 0–3; null without a sample |
| `timestamp_ticks` | Actual nonzero SSC timestamp; null without a sample |
| `dsp_clock_hz` | Null: this endpoint's tick frequency is unverified |
| `dsp_clock_mapping` | `unverified`; no conversion or host-clock synchronization is implied |
| `received_boottime_ns` | Host `CLOCK_BOOTTIME` receipt time in nanoseconds; null without a sample |
| `published_boottime_ns` | Host `CLOCK_BOOTTIME` snapshot publication time in nanoseconds |
| `freshness_window_ns` | 30000000000: a 30-second receipt-recency window |
| `units`, `calibration` | Both `unverified` |

Status 0 is retained as an unreliable raw observation with `available=false`.
Status 3 is not a calibration certificate. An on-change value can remain stable
without a new event; after 30 seconds it is marked stale for consumers, without
inventing a sensor fault or requesting a new sample. Stale values remain visible
with `has_sample=true`, `fresh=false` and `available=false`. A transport/reset
discontinuity clears values, status and timestamps entirely.

A reader must use a bounded regular-file read, verify the root-owned directory
and file, reject symlinks and unexpected schema/types, and recompute freshness
using its own `CLOCK_BOOTTIME`. Reject future receipt/publication times, a
publication older than five seconds, or a receipt older than the advertised
30-second window. This also invalidates an abrupt publisher death; a cached
Boolean alone must never establish liveness. Host timestamps include suspend
time and must not be compared with wall time or the unverified DSP clock.

## Source and validation boundaries

The implementation reuses the existing native SSC client/parser from support
commit `2676cdd47dc33da661315091f599587396e3b288`. The active Uke HAL uses 1025;
the generic `sns_cct.proto` event 882 is not substituted. Qualcomm's pinned
[client API](https://github.com/qualcomm/sensinghub/blob/329551963bcc063f6a89f6cbca259c5b5c9c4e0b/apis/proto/sns_client.proto)
leaves sensor event timestamps sensor-specific. A generic libssc 19.2 MHz
comment does not independently establish the CCT endpoint's clock frequency.

Each datagram is limited to 16 KiB, each client event envelope to 32 entries,
each sample vector to 16 finite floats, and receipt rate to 64 datagrams per
second. Validate QMI peer/client identity, selected SUID, message ID, status and
strictly increasing nonzero timestamps. Discovery and acknowledgement have
five-second/64-packet bounds. No raw payload or sensor identifier is logged.

`make test-front-color` compiles the publisher and runs isolated host fixtures
for metadata/type refusal, sample framing, malformed values, status, stale and
reset state, atomic replacement and symlink/hardlink/permission refusal.
`make front-color-srpm` creates the separate source RPM. Host checks, COPR,
package payload/dependency checks, enforcing SELinux, explicit activation,
continuous reception, stop/crash invalidation and physical color response remain
coordinator acceptance gates. None is inferred from source preparation.
