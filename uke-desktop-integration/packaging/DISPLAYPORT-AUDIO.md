# DisplayPort audio package

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
