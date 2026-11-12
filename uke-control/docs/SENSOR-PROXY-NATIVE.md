# Original Fedora SensorProxy qualification

12 November 2026. The installed runtime remains Control2.17.0-12,
uke-iio-sensor-proxy3.9-24 and uke-libssc0.4.4-26 on kernel7.3.3-24-unstable.
Original Fedora FastRPC is already used by Control. This record concerns the
separate desktop sensor provider; it does not retire either remaining sensor RPM.

## Source and package identity

The refreshed Fedora repository still supplies the original
`iio-sensor-proxy-3.9-3.fc45.aarch64` package. Its verified RPM SHA256 is
`fea7da87f23abb40be235326c1f6dec73786609dd30463bd37cc734eaaa81416`;
the Fedora signature fingerprint is
`d924b10d3e810dabdd8b56b596e7e91491211fce`. The executable matches the signed
RPM file digest. No Fedora binary or source is rebuilt or patched.

The GPL-3.0-or-later upstream3.9 archive SHA256 is
`19a4ae99b1b8af603fc789841025f9747bf445418d69ec7389e75adb6c8186bf`.
The current upstream head is `ca0d5b08bd647afc5f8ead3ce00a44018fbc633f`;
the comparison with3.9 changes only `.gitlab-ci.yml`. Fedora rawhide source
revision `fe0de0f1d4ec7fdc006a087167751a7a334eb6e5` disables the SSC backend.
Relevant original files are `src/iio-sensor-proxy.c`, `src/drivers.c`,
`src/drv-iio-poll-light.c`, `src/drv-iio-poll-proximity.c`,
`src/drv-input-proximity.c`, `src/drv-iio-buffer-compass.c` and
`src/iio-buffer-utils.c`. The existing kernel source and protocol attribution
remain in the kernel's `SSC-MAGNETIC-HEADING.md` and native sensor manifests.

The active upstream queue includes proximity allocation MR419, availability
notifications MR418/414, a boot race MR415 and multiple-device draft MR378.
Their existence does not establish inclusion in Fedora3.9 or Uke acceptance.
No patch from those queues is copied into a project rebuild.

## Bounded device result

A temporary mount namespace changes only the test process's udev database view.
It removes desktop classifications from raw front/rear ALS and binary raw IIO
proximity. All original sysfs interfaces remain. The signed original executable
then selects the buffered native accelerometer, `uke-ssc-dual-light` and the
native `SW_FRONT_PROXIMITY` input device. A12-second original monitor client
successfully claims those three sensors and reports lux units without a claim
error. This verifies stationary API delivery, not physical orientation, near/far
transitions, lighting accuracy, suspend, or an installed provider transition.

Compass discovery fails: the standard native heading buffer has no data during
the original driver's0.5-second probe. A separate direct heading read also times
out. The retained libssc CLI produces12 finite compass notifications during a
two-second request. That CLI's `report_received()` accepts four quaternion
components without checking the protobuf accuracy field, whereas the native
driver retains its reliability gate. Historical kernel17 status-only tracing
found unreliable ROTV/geomagnetic reports; that is not a measurement of current
kernel24 report accuracy. Current metadata discovery returns one ROTV candidate.
The underlying request, timing and quality difference remains unresolved.
Do not bypass the firmware gate or infer that the dock causes this result.

The first trial stops before claims when it requires HasCompass. A second trial
explicitly records partial acceptance and exercises the other three claims.
Both restore the existing provider automatically. All seven relevant unit
active/enabled states match their initial values, including Adaptive active but
disabled. ADSP/WPSS stay running with zero crashes and CDSP offline with zero.
The private FastRPC mount remains noswap. No package, kernel, slot, persistent
udev rule, display preference or private calibration is changed.

## Reproducible fixture and remaining transition

`tests/sensor-proxy-native-live.sh` prepares the same bounded test from an
original RPM/extracted reference directory, a fresh output directory and the
coordinator's delivery lock. It verifies the signed executable, uses a private
udev view, records only selections/counts and arms an independent restore timer.
The published form adds explicit stop and buffer-restoration verification to
the tested local prototype; those added failure branches are source-prepared,
not an additional device acceptance claim. It is a manual qualification tool,
not an installed helper or runtime provider.

Production routing must cover initial and late combined-ALS registration,
partial probe, removal, restart and old-kernel fallback. The existing
`ssc_proximity_input_abi=1` capability permits early classification of the
native input contract without racing its later input event. A registered
combined ALS alone does not make the stock proxy prefer it over an existing
raw ALS provider. Keep desktop selection integration in Control and the
protocol/lifecycle in kernel/DT; add no runtime RPM or idle polling.

Control currently requires `iio-sensor-proxy(ssc)`. A future cumulative update
must change that dependency only after complete parity, retain all actual
Control12 inputs and cache/downgrade behavior, qualify signed DNF upgrade,
reinstall and rollback, then retire the custom proxy/libssc producers. The
libssc package also owns `ssccli`; preserve its supported diagnostic functions
through existing native interfaces rather than silently dropping them.

Primary references: [upstream](https://gitlab.freedesktop.org/hadess/iio-sensor-proxy),
[Fedora package](https://packages.fedoraproject.org/pkgs/iio-sensor-proxy/iio-sensor-proxy/fedora-rawhide.html),
[Fedora source](https://src.fedoraproject.org/rpms/iio-sensor-proxy).
