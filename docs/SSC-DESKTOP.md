# SSC desktop integration

17 October 2026: fileservice release 4 adds systemd readiness after successful
FastRPC listener registration. Device presence and the read-only mount precede
the root listener; the sensor listener precedes the original SensorProxy D-Bus
service. Adaptive-display release 2 attaches its explicitly enabled target to
the FastRPC device. No fixed delay establishes readiness. Native54 lacks the
upstream FastRPC DT binding; use a separately qualified matched candidate before
activation. The accepted .4054 boot and its module fallback remain preserved.

13 October 2026. A bounded Xiaomi Pad 7 test received stationary orientation
through the upstream SensorProxy D-Bus interface with SELinux enforcing.
No permanent sensor service or complete sensor RPM is released.

Use upstream libssc 0.4.4 and iio-sensor-proxy 3.9. Exact source and host SDK
identities are in [the source manifest](../manifests/ssc-desktop-source.json).
Reference archives remain unchanged; apply the patches to a development copy.

- Build libssc with `-Dtests=false`. Its Python mock servers are excluded;
  Meson is a documented host-only Python dependency.
- Build iio-sensor-proxy with `-Dssc-support=enabled -Dtests=false
  -Dgtk-tests=false -Dgtk_doc=false`. Fedora 3.9-3.fc45 disables SSC in its spec.
- Apply the startup patch before testing early clients. SSC discovery dispatches
  the main context; sensor drivers must be open before the D-Bus name is owned.
  This follows [GIO's name-ownership contract](https://docs.gtk.org/gio/func.bus_own_name.html).
- The candidate SELinux policy grants only the SensorProxy domain access to its
  own QRTR datagram sockets. It grants no device, calibration or firmware access.
  `/run` is `nosuid`: a binary there does not reproduce an installed executable's
  normal SELinux domain transition. Test the actual service context explicitly.

`sh tests/iio-startup-hil.sh PROXY MONITOR LIBDIR` is a bounded, manual laboratory
regression check. Run it only after firmware, the read-only factory calibration
mount and the native fileservice have been admitted, with the distribution proxy
stopped and no overlapping test. It starts the client before the daemon, requires
a real orientation event and cleans up its processes. Logs remain private.
`tests/qrtr-socket.cpp` distinguishes socket availability from sensor discovery;
a successful socket is not a measurement or calibration result.

The original proxy failed the early-client trial; the patched binary passed.
The confined service passed only while the narrow policy was loaded. The policy
was then removed and the denied socket check repeated. Factory persist stayed
read-only with an unchanged whole-partition hash.

Remaining work: matched firmware and fileservice lifecycle, front-light and
magnetometer discovery, rear-light units, controlled axes, desktop rotation,
cold boot, restart and suspend/resume. Do not copy another device's mount matrix
or enable a permanent service from this laboratory result.
