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
