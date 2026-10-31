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

The gain controls have separate roles. The Uke static mixer initializes all
four ADC volume indices to6. Selected single-microphone paths set their TX
decimators to102, which the native TX control describes as+18dB. Other paths
use different gains, including71 for selected unprocessed inputs. These are
route-specific settings; copying the highest value into every Linux route
would not reproduce the Android processing chain.

Native Linux7.3.1, the Uke MiCode driver and the reviewed same-SoC OnePlus
driver expose matching codec ADC controls, range0..20, using the same analog
register fields. Their TLV metadata advertises0.25dB per ADC index. That
metadata is not an independent measurement of the analog response. The
separate TX control is digital, with1dB steps from-84 to+40dB.

Policy12 selects the Uke ADC baseline6 and TX index96(+12dB), retaining the
native ADC_NORMAL mode, channel order and electrical settings. It does not
request desktop software amplification. This is a gain-policy candidate;
package parsing and source comparison do not establish speech level, clipping
margin or SNR improvement. Those require separate device acceptance.

The supported HiFi profile exposes four named mono inputs and both outputs.
The distribution's Pro Audio profile bypasses that UCM layout and presents
one four-channel hardware input with generic names. Profile changes reapply
the packaged verb's gain values; transient mixer changes are not an installed
policy update. No profile override service or KDE modification is required.

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
- [Uke MiCode WCD939x](https://github.com/MiCode/vendor_qcom_opensource_audio-kernel/blob/8eb0eccbc4027ae4a4987e4bced03a4381039714/asoc/codecs/wcd939x/wcd939x.c)
  and its register header, GPL-2.0-only, Qualcomm/Linux Foundation attribution.
- [Same-SoC OnePlus WCD939x](https://github.com/OnePlusOSS/android_kernel_modules_and_devicetree_oneplus_sm7675/blob/1138e00fee63582a0cc6a53af3286621786827b7/vendor/qcom/opensource/audio-kernel/asoc/codecs/wcd939x/wcd939x.c),
  GPL-2.0-only. Shared-IP control semantics, not portable board routing.
- [Native WCD939x](https://kernel.googlesource.com/pub/scm/linux/kernel/git/stable/linux/+/79a907609d53d5cd645b42c11550d295d43abf99/sound/soc/codecs/wcd939x.c)
  and reviewed current upstream36aad21d50d1af3f6954bb0c988257a908202477.
