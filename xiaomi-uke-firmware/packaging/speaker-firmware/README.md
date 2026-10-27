# Uke speaker preset

27 October 2026. Source and package delivery for Fedora Rawhide AArch64;
physical speaker acceptance remains open.

This package installs the unmodified generic FS19xx preset from Uke Global
OS3.0.304.0. The original ODM notice explicitly maps
`/odm/firmware/fs19xx.fsm` to content ID
`3b83ef96387f14655fc854ddc3c6bd57`, which contains Apache License 2.0.
The complete compressed notice and decoded license are retained in the RPM.
This file-specific review does not extend to other OEM firmware.

The driver requests `qcom/uke/audio/fs19xx.fsm`. Package installation supplies
that path; it does not unmute hardware, run calibration, reload a module or
reboot. The matching kernel and UCM profile activate the selected playback
path after the normal kernel reboot. Existing modified presets are preserved
with RPM noreplace semantics.

Build on the host with `make speaker-srpm SPEAKER_PRESET=... SPEAKER_NOTICE=...`.
Inputs must match the exact identities in provenance.json. The source archive
contains no persist data, unit calibration or proprietary executable.
