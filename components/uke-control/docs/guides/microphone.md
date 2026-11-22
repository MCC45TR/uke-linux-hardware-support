# Internal microphone selection

31 October 2026. POCO Pad X1 / Xiaomi Pad 7, Fedora Rawhide AArch64,
PipeWire 1.6 and WirePlumber 0.5.16 or later. This candidate adds control and
routing source; physical and power acceptance of the new policy is pending.

The existing device-named four-channel input remains the public input.
Kernel-core continues to own UCM, channel layout and startup gain. Control
does not change ADC, bias, supplies, SoundWire, firmware or boot partitions.
No separate microphone RPM or root service is added.

```console
uke-control microphone status --json
uke-control microphone list
uke-control microphone select uke-amic1
uke-control microphone select auto
uke-control microphone processing status --json
```

Selection is saved through WirePlumber's normal user settings. An explicit
manual preference applies to new implicitly routed mono/stereo streams.
Already open recordings retain their original links; reopen recording to
apply a changed preference. Explicit application targets, four-channel raw
recording, Production streams and external microphones bypass the selector.
Applications may also set `uke.microphone.raw=true` on their own stream.
`auto` preserves the ordinary raw path because no automatic profile is
acoustically calibrated. No application is classified by process name.

| Identity | Raw PCM index | Aperture in the documented reference frame |
|---|---|---|
| uke-amic1 | 0 | Top edge, right |
| uke-amic3 | 1 | Left edge |
| uke-amic4 | 2 | Beside camera; front/rear association unmeasured |
| uke-amic5 | 3 | Top edge, left |

Reference: screen facing the user, landscape, USB-C at the right. Physical
identities and raw channel order do not rotate. The installed kernel layout
is the source of listing data; Control does not install another copy.

The selector accepts exactly one ALSA source with `mic:uke-4ch-v4 kernel:RELEASE`,
the matching model and AUX0–3 in order. Running release102 advertises v3 and
is intentionally unsupported for this new policy. The signed105/106 kernel
candidate needs separate boot and microphone acceptance. Missing, ambiguous
or mismatched sources retain ordinary raw capture and report unsupported.

An explicitly selected channel uses a passive PipeWire loopback in the
existing WirePlumber client context. Its internal source is hidden from
normal input enumeration and uses no ALSA device opener of its own. Mono
samples are copied to both stereo ports for ordinary applications. This is
sample routing, not beamforming or speech enhancement. It is not a claim
that selecting one channel switches off the other physical ADCs.

Default `auto` creates no additional audio path. A manual path is passive
while unused. Existing selected recordings retain their path until they
close. No timers, sensor claims, camera discovery, recording files or network
transport are introduced. CPU, energy and lifecycle acceptance are reported
as **not measured** until a coordinated tablet comparison is performed.

| Requested feature | Candidate status | CPU / energy acceptance |
|---|---|---|
| Saved manual channel preference | Implemented; device acceptance pending | not measured / not measured |
| Raw access and explicit target precedence | Preserved by policy guards; regression acceptance pending | not measured / not measured |
| Orientation selection | Disabled; four orientation profiles uncalibrated | not measured / not measured |
| Camera/call profiles | Context parser only; profile application disabled | not measured / not measured |
| Occlusion, AEC, NS, AGC | Unsupported; no admitted processing chain | not measured / not measured |
| Beamforming, RNNoise, ML | Excluded | not measured / not measured |

Applications can describe their own stream with `uke.microphone.camera`
(`front`, `rear`, `none`) and `uke.microphone.purpose` (`call`, `scene`,
`narration`, `recording`). These properties are context, not identity,
authorization or calibration. They never change another client's source.
Unknown context preserves normal routing. No new D-Bus service is introduced.
The existing speaker orientation helper is not modified while profiles are
uncalibrated; no new accelerometer subscription or polling is necessary.

The native Control TUI exposes microphone status and the same five selection
choices under Audio, camera and NFC. The existing KCM remains a read-only
overview; manual control belongs to the TUI/CLI. This candidate does not fork
or replace KDE applications. Processing enable
requests return an explicit unsupported error rather than enabling an
unmeasured effect or claiming success.

## Validation and rollback

Compile and execute all fixtures only on COPR. Pure policy fixtures cover
model/marker/channel rejection, manual precedence, explicit/raw/external
fallback and unknown camera context. Native fixtures cover bounded JSON,
duplicate keys, malformed marker/order and unsupported-effect refusal.
Fixture success does not establish physical audio, idle power or capture
shutdown. The tablet checklist remains: idle capture/power, all four manual
choices, mono/stereo and four-channel raw recordings, simultaneous clients,
mute, external device changes, reopen and suspend/resume. Compare at least
ten-minute raw/manual intervals; keep effects disabled without acceptance.

To return to ordinary capture, run `uke-control microphone select auto`,
then close selected recordings. Do not restart the audio session during an
active recording. If Control cannot run, `wpctl settings --save
uke.microphone.selection '"auto"'` restores the same preference. Removing
the Control-owned policy files leaves kernel-owned raw capture available.
No gain restoration, kernel rollback or partition operation is needed for
this userspace selection rollback.
