# Microphone routing and speech processing candidate

3 November 2026: the source candidate adds passive WebRTC echo cancellation,
noise suppression and high-pass filtering to new eligible internal-array capture
streams. Installation and physical speech quality remain separate acceptance
steps. Speaker gain and physical microphone gain are unchanged.

Use the shared terminal actions under Audio, or the existing command-line client:

```sh
uke-control microphone status
uke-control microphone streams
uke-control microphone processing verify
uke-control microphone processing select calls
uke-control microphone processing select all
uke-control microphone processing select off
uke-control microphone processing agc off
uke-control microphone select uke-amic4
uke-control microphone select auto
```

`calls` is the default: Communication streams and explicit call/narration context
receive speech processing. `all` includes ordinary implicit mono/stereo captures.
`off` retains raw/manual routing. AGC defaults off; enabling it changes only
WebRTC's digital processing. Existing recordings retain their selected path;
preferences apply to new streams. Explicit targets, external microphones,
Production streams and raw four-channel access are preserved.

Auto selects amic1/AUX0 as a deterministic speech reference. It does not choose
an acoustically optimal microphone or implement calibrated beamforming. Manual
selection chooses one fixed physical channel. The public raw source keeps its
four AUX channels; hidden session-owned paths handle selected/speech capture.

`streams` reads graph metadata without starting capture. It reports actual linked
application ports, upstream physical channels and the AEC monitor-reference state.
Disconnected, ambiguous or foreign paths remain unverified. App-internal mixing
and app-internal effects cannot be inferred. Requested policy, running linked
paths, available libraries and actual engine execution are distinct evidence.

`processing verify` runs the installed SPA WebRTC implementation on bounded
synthetic memory buffers. It checks finite nonzero signal response, negotiated
buffer contracts and DC rejection without opening audio hardware or saving audio.
This is not an acoustic echo-convergence, intelligibility, latency or power test.

The retained COPR source producer overlays the microphone/catalog changes onto
control 2.8.1-7 after verifying the exact original file hashes. It preserves that
cohort's newer backend, TUI and frontend contracts. Development and installation
use COPR and DNF; no native tablet compilation or copied binary installation is
part of this workflow. The separate check package carries only a source receipt.

Stock DSP identifiers and media-format headers are protocol evidence. They do
not establish that a proprietary firmware algorithm exists, accepts its tuning
or runs. This candidate uses the distribution's executable PipeWire/WebRTC path.
See the canonical lesson `UKE-AUDIO-SPEECH-20261103-01` and the earlier speaker
VI/gain lessons for independent protection and physical acceptance gates.

| Identity | Raw PCM index | Physical aperture |
|---|---|---|
| uke-amic1 | 0 / AUX0 | Top edge, right |
| uke-amic3 | 1 / AUX1 | Left edge |
| uke-amic4 | 2 / AUX2 | Beside camera; front/rear association unmeasured |
| uke-amic5 | 3 / AUX3 | Top edge, left |

Reference frame: screen facing the user, landscape, USB-C at the right.
Identities and raw channel order do not rotate. Kernel-owned installed layout
metadata supplies the list. Exactly one model-matched ALSA source with
`mic:uke-4ch-v4 kernel:RELEASE` and AUX0–3 in order is required. Older markers,
missing/ambiguous arrays and mismatched layouts preserve ordinary routing.

Applications may describe context with `uke.microphone.camera` (`front`, `rear`,
`none`) and `uke.microphone.purpose` (`call`, `scene`, `narration`, `recording`).
These do not authorize changes to another client's source. No process-name
classification or orientation polling is used. Occlusion/orientation selection,
calibrated beamforming and automatic camera profiles remain unavailable.

Physical validation still needs idle capture/power, all four channel choices,
mono/stereo and four-channel raw capture, simultaneous clients, mute, external
source changes, reopen and suspend/resume. The 31 October source record's raw
routing checks did not establish those physical outcomes; new speech processing
adds separate echo, intelligibility, latency and energy acceptance requirements.

To restore ordinary capture, select processing `off` and microphone `auto`, then
close selected recordings. Existing recordings keep their path until closed.
Do not restart the audio session during recording. The same saved settings can
be restored with `wpctl settings --save uke.microphone.processing '"off"'` and
`wpctl settings --save uke.microphone.selection '"auto"'`. No kernel rollback or
partition operation is required for this userspace preference rollback.
