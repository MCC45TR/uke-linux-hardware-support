# Native proximity selection

The original Fedora SensorProxy 3.9 probes IIO before input. Its integer
threshold backend selects Uke's raw binary proximity channel, then fails to
open it because a nonzero integer threshold cannot represent this `0/1`
contract with hysteresis. A reversible native-device trial selected the
existing `SW_FRONT_PROXIMITY` input successfully after excluding the raw IIO
poller. The raw values remain part of the public kernel ABI and must not be
renumbered or hidden from general applications.

`packaging/89-uke-native-proximity.rules` changes only SensorProxy's discovery
property when the kernel explicitly advertises `ssc_proximity_input_abi=1`.
The capability is created with the IIO device before its add event. The proxy can then
discover input at startup or its subsequent add notification. A child-path
existence test alone would race input registration; an unversioned device-name
override would break old kernels. Kernels without the capability keep their
existing classification and rollback behavior. No helper process, service,
polling, threshold, sensor request or hardware protocol is added.

This rule is prepared source and is not yet part of a shipped Control RPM.
Its cumulative source/package transition and the kernel capability require
qualification. Original-proxy magnetic heading, physical proximity and both
ALS/rotation/power/suspend parity remain separate acceptance requirements.
The custom proxy and libssc must remain until those transitions are complete.
