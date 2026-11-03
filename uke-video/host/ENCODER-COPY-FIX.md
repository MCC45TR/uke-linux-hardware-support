# Synchronized encoder staging on POCO Pad X1

3 November 2026. The encoder now prefers the existing CPU copy for its private
staging allocation. V4L2/Iris still performs hardware encoding; direct input
imports, allocation, fence waits and buffer lifetime handling are preserved.
The existing explicit `IRIS_VAAPI_COPY` diagnostic override remains available.

On Linux 7.3.1-1011 with the preceding signed backend, the same 30-frame H264
test produced 16.194250 dB PSNR through GPU staging and 46.542374 dB through
the CPU-copy control. Source/build results for this package are separate from
the installed candidate's own image-quality and decoder regression tests.

This uses the per-copier fallback already present in the retained decoder
policy. It does not identify or repair a general kernel coherency defect.
Full codec/profile, browser, lifetime and sustained-performance qualification
remains separate. The original README describes the preceding encoder policy;
this note supersedes that default for this package revision.

## Packaged GStreamer admission

This package supplies `GST_VA_ALL_DRIVERS=1` in the vendor systemd user
environment, standard login-shell settings and a system-wide Plasma environment
hook. It requires the original Fedora `gstreamer1-plugins-bad-free` package
that owns the modern VA plugin. These settings are installed and removed by
RPM; no per-user export, local wrapper or modified GStreamer is required.

The unchanged session package's standard systemd RPM triggers reload active
user managers during a normal DNF upgrade. New logins load the packaged defaults
through their normal startup paths. Already-running applications retain their
old process environment until restarted. The existing CPU encoder staging
correction remains in this release.
