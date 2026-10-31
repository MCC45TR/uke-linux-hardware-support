# DisplayPort audio package

## Raw capture channel gate, 31 October 2026

Release 0.1.0-10 requires `mic:uke-4ch-v3`. Release101's corrected close still
uses a quad-speaker map in the frontend converter: individual attenuation
shows one input mixed into the first pair, another moved to the third output
and a silent fourth output. The new kernel candidate preserves raw codec-DMA
IDs through the frontend. Desktop configuration must require that correction;
four selectable sources alone do not establish four independent microphones.

## Four microphone sources, 31 October 2026

Release 0.1.0-9 defines Microphone 1 through Microphone 4 as separate mono
capture devices. PipeWire's native UCM split metadata shares one four-channel,
48 kHz capture PCM and selects indices 0 through 3. Closing one logical source
does not disable the shared hardware routes. Gains and mute remain independent
software controls. Source data requires `mic:uke-4ch-v2` from the corrected
native capture driver; earlier cards retain the existing playback profile.

The proposed stream order is AMIC1, AMIC3, AMIC4, AMIC5. Physical aperture
associations remain unknown until independently checked on POCO Pad X1.
No speaker-position labels or Android geometry/calibration are assigned.
Four speaker channels and stereo DisplayPort remain separate output devices.
COPR parser/sample checks and installed tablet acceptance are distinct results.

Native UCM split fields follow PipeWire source
`4cfc471123a0e041fcd6fec5887c40dbddd40325`,
`spa/plugins/alsa/acp/alsa-ucm.c`. The conditional card capability follows
ALSA-lib `f44e0abd5382f519cc6fa69850c34e6e9c09d5f5` UCM conditions.

## Separate four-channel speakers and stereo DisplayPort, 27 October 2026

Release 0.1.0-7 exposes four channels on the existing MultiMedia2 internal
speaker PCM. ALSA preserves all four input samples, with 48 kHz/S16_LE format
conversion. MultiMedia1 and DisplayPort remain a separate two-channel output;
their ELD, jack and routing controls are retained. No output combines tablet
speakers with an HDMI monitor. Acoustic slot independence and spatial order
require separate tablet observations.

## Combined speaker card, 27 October 2026

Release 0.1.0-6 adds `POCO-Pad-X1` for Linux 7.3.0-21 while retaining the
existing HDMI-only card. Stereo input is converted to 48 kHz S16_LE and routed
to the measured TDM slots as left/right/left/right. The four amplifier
switches follow speaker-device selection; hardware gain is not raised.
DisplayPort retains PCM 0, its mixer and jack. No capture device is advertised.

The separately provisioned OEM amplifier preset and qualified hardware are
required. No tuning/calibration payload, daemon, startup hook or KDE fork is
added. Host checks parse the shipped ALSA data and exercise the route plugin
against regular files. New-card discovery, audible channels, HDMI coexistence
and suspend require physical validation on the final package pair.

The following dated history applies to the original HDMI-only profile.

On 24 October 2026, Fedora 46 Rawhide AArch64 on POCO Pad X1 with kernel
7.2.12-28 has no connected DisplayPort playback PCM. Kernel candidate
7.2.12-30 adds the single digital frontend and DP104 backend. Its ALSA card
driver is `sm7675`; the card name is `POCO-Pad-X1-HDMI`.

`uke-displayport-audio` supplies only the matching ALSA UCM data. The device
name `HDMI1` follows the UCM desktop convention; its actual tablet transport
is DisplayPort. Juo JH925 converts that stream to HDMI. The profile selects
PCM 0, enables its DP mixer and follows `DP0 Jack`. It contains no internal
amplifier, codec initialization, capture, daemon or boot hook.

Original Fedora alsa-ucm 1.2.16.1 provides the discovery and shared syntax.
The installed X1E80100 Elite-14 profile is a shared AudioReach control and
DP-jack comparison, not a board configuration copied to Uke. Exact Uke OEM,
same-SoC and kernel source pins are in the workspace DisplayPort audio lesson.

Build a source RPM from the component directory:

```sh
make displayport-audio-srpm
```

Source, package and parser checks do not establish monitor sound. Physical
acceptance requires the new card, DMA domain, DSP graph, monitor jack and
audible output on the tablet. Cover/USB stability remains separate.

The first physical release-30 boot registers the expected card, PCM, DP jack,
monitor ELD and DMA domain. The initial UCM package parses but desktop probing
rejects the PCM while its sole backend is disabled. Release 0.1.0-2 moves that
mixer sequence to verb activation, before the PCM probe. Playback and audible
output remain separate acceptance gates.

Release 31 plays 48 kHz/16-bit stereo directly through DP to the Juo HDMI
monitor. The operator confirms the test tone. Desktop routing initially stalls
in WirePlumber camera creation for the protected NT36532 raw THP interface.
Release 0.1.0-3 excludes only its exact SPI bus path and raw product name from
camera discovery. Raw access protection and USB cameras remain unchanged.
A live configuration trial restores automatic stereo ports and completes
PulseAudio playback. Browser playback and monitor naming need separate checks.
