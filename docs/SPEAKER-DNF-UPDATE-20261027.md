# Internal speaker update delivery

27 October 2026. Target: Fedora Rawhide AArch64 on Uke with
`mcc45tr/uke-linux-test` already enabled and `uke-core-meta` installed.

The previous kernel only recommended its UCM and orientation packages. The
generic firmware RPM also omitted the FS19xx preset requested by the speaker
driver. An ordinary update could therefore leave the required playback path
incomplete, especially with weak dependencies disabled.

`uke-core-meta` 1.0.6-5 requires `uke-sound` 1.0.5. That selection requires the
7.3.0-33 or later kernel, UCM policy 0.1.0-8 or later, orientation helper
0.1.0-5 or later, and `uke-speaker-firmware` 3.0.304.0-2 or later. These are
children of the sound source family, including the retained PipeWire recipe.
The firmware
package supplies the exact stock preset and OEM notice. Its admission is
file-specific; other firmware reviews are unchanged.

Update through the normal repository transaction:

```sh
sudo dnf update --refresh
```

Reboot normally to use the matched kernel/modules and probe the newly supplied
preset. No installer, mixer command, module reload, calibration command or
manual firmware copy is required. Existing modified presets use RPM noreplace
semantics. The active desktop selects the internal Speakers output through its
standard audio controls; external-output routing remains available.

Source/package checks and an offline upgrade model are recorded separately from
physical acceptance. Speaker playback, protection effectiveness, channel order
and suspend/resume still require tablet observations. Microphone capture is not
added by this delivery.
