# SSC sensor packages

14 October 2026. Fedora Rawhide AArch64 preview sources:

- `libssc`: native SSC client library; Python mock servers are excluded.
- `iio-sensor-proxy`: upstream desktop D-Bus API with SSC enabled and the
  previously checked startup-order correction. A narrow SELinux module permits
  its own QRTR sockets.
- `uke-ssc-fileservice`: native FastRPC file transport with private RAM registry
  storage. Symlink escapes and incomplete shell inputs are rejected.

`make srpm PACKAGE=NAME` downloads the exact upstream archive below
`referances/`, verifies SHA-256 and packages the attributed patches. COPR binary
workers build without network access. Upstream C code retains its native build;
new protocol fixtures use C++ where applicable.

Upstream Meson needs Python on the host; replacing the build engine is outside
this packaging change. BuildRequires pins Python `3.15.0-1.fc46` and Meson
`1.12.1-2.fc46`. No Python interpreter, script or mock server is installed in
the target payload. Build-time host tools do not imply target dependencies.

The file-service package requires separately admitted device-local firmware
and configuration. Factory persist remains read-only; copies and any registry
updates stay in `/run/uke-ssc`. Calibration and proprietary data are not COPR
inputs. No unit is independently enabled by an install preset.

The optional `uke-adaptive-display` integration target supplies an explicit
activation path. See its [device trial guide](../../uke-desktop-integration/docs/ADAPTIVE-DISPLAY.md).
Physical orientation, front-light discovery, lux response, cold boot and
suspend/resume remain unqualified by package builds.
