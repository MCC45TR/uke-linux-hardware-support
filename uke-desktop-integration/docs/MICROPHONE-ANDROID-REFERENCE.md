# Android microphone routing reference

31 October 2026. POCO Pad X1 configuration research alongside Fedora Rawhide
AArch64, Linux7.3.1-102. This is source evidence; Android runtime recording and
effect selection were not measured. No Android firmware, DSP preset or private
calibration is installed by the Linux audio policy.

The Uke resource manager declares one handset-microphone device with a four
channel maximum and a two-channel, 48 kHz, 16-bit default. Named single-input
recording presets select one channel. Voice-call presets provide dual, triple
and quad microphone routes. Raw, deep-buffer and compressed HDR presets can
select four channels, with separate orientation-specific route names. VoIP and
voice-call entries enable the echo-cancellation configuration flag. Those
entries describe available routes, rather than the route chosen by every app.

The stock common microphone-characteristics file declares four physical
microphone records. Its active-device metadata associates handset capture with
one record, speaker capture with two, voice-activation capture with three and
ultrasound capture with four. These entries are all marked processed. This is
an untailored shared configuration: the ultrasound entry does not establish
Uke ultrasound support, and the repeated geometry is not accepted as measured
Uke geometry. Linux Microphone1-4 are not assigned Android builtin_mic IDs.

Android separates input-device enumeration, physical microphone information
and the microphones contributing to an active stream. AOSP exposes static
microphone information through IModule.getMicrophones and dynamic stream
information through IStreamIn.getActiveMicrophones. The public MicrophoneInfo
channel map distinguishes direct samples from processed contributions. A
processed channel can contain more than one physical microphone. Consequently,
seeing fewer input-device names does not establish that physical microphones
are absent or inactive. The actual app, audio source and vendor route must be
observed to determine the active set.

Linux currently preserves one raw four-channel capture PCM and exposes four
independent mono views for physical identification. The packaged layout links
those views to AMIC1/3/4/5 and qualitative aperture locations. Android-inspired
processing remains a later step; metric geometry, effect ABI/licensing and
actual processing behavior require separate qualification.

Sources:

- Uke device commit0f03fdc4432a86e8aefa0b71d395391f18dc8ad5,
  configs/audio/resourcemanager_cliffs_mtp.xml, SHA256
  ec2fd1eea221ff0d7ade65f6054055fc3bb65dc4cc95c9914b1a75a321173e7b.
- Uke OEM static mixer commit87f59fc5c2ef1ee04e494e5258853d36a3346059,
  proprietary/odm/etc/audio/sku_cliffs/mixer_paths_overlay_static.xml, SHA256
  2cb2cfc48ff22b76e67783753203a0367f4e447b398db6d7eb61ef5a2e8a8b4c.
- Common vendor commit475e955b3780aee6dd007178e08bf8a231143681,
  proprietary/vendor/etc/microphone_characteristics.xml, SHA256
  618133e7ddd73dba10d45036856127b91b9e649299e1fd8b30fcb2e1002744bb.
- [AOSP IModule](https://android.googlesource.com/platform/hardware/interfaces/+/5688f7eb1e117ed26e642a695de300b7683acb87/audio/aidl/android/hardware/audio/core/IModule.aidl)
  and [IStreamIn](https://android.googlesource.com/platform/hardware/interfaces/+/5688f7eb1e117ed26e642a695de300b7683acb87/audio/aidl/android/hardware/audio/core/IStreamIn.aidl).
- [Android MicrophoneInfo](https://developer.android.com/reference/android/media/MicrophoneInfo#getChannelMapping()).
