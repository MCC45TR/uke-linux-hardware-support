# OSS normal-range brightness preference

The kernel owns both calibrated light channels, cover selection and the native
backlight/LUT. A user's lux-to-brightness preference belongs to the original
desktop. This one-shot Control operation imports the reviewed OSS normal-range
curve through KWin's existing output configuration; it adds no daemon, sensor
reader, periodic wakeup, runtime RPM or desktop fork.

`uke-control desktop brightness-curve status` reports the saved point count,
pending request and restore availability. It does not claim that the active
compositor has loaded that saved curve.

`uke-control desktop brightness-curve queue-oss` requests an import at the next
fresh Plasma session. The current session is unchanged. A startup hook runs
only while a request exists, before the original compositor starts. The native
operation refuses an active KWin/Mutter owner or unavailable user bus, a changed
curve/panel identity, another board, an ambiguous internal panel or a native backlight
range other than 0..2047. A refused request remains pending and is logged.

Only the unique internal output's `autoBrightnessCurve` changes. Brightness
enablement, rotation lock, modes, VRR, ICC/HDR and external layouts are preserved.
The old curve is retained in private user settings.
`queue-restore` requests restoring that curve at a fresh session; it refuses to
overwrite later user learning. `cancel` removes an uncommitted request. Startup
publication is atomic and an interrupted committed transition can be completed
without importing it twice. No firmware or private calibration is copied.

Shutdown may save brightness, DDC detection and external display layouts after
the request was queued. A pending transition checks the original curve and
internal panel identity, then merges only that curve into the current document.
Unrelated changes survive. New learned curves still prevent an import/restore.
Both compositor-absence checks and the final file-concurrency check remain.
Startup diagnostics go directly to the journal under `uke-brightness-curve`;
Plasma's environment-script subprocess otherwise discards standard error.

The numerical lux/nits table comes from the Apache-2.0 common overlay,
SPDX-FileCopyrightText: The LineageOS Project, at
`Xiaomi-Pad-7-Pro-Resources/android_device_xiaomi_sm8635-common`, commit
`8bd8e52a0245178256b70d5f8f093a0620ca8ce7`, path
`overlay/FrameworkResOverlayCommon/res/values/config.xml`, blob
`7d07f3da845e46211b1201aa1fa993aa2020e873`. Android 16 `Spline.java`,
Copyright 2013 The Android Open Source Project, supplies the monotone
interpolation model under Apache-2.0; the existing project's host converter is
the implementation reference.

Public product-map numerical points are (2 nit,0.001709819),
(600 nit,0.49975574), (800 nit,1); source vendor commit
`6a411afd5b39f7a61a44cc64f0f16d1fdf3fdb95`, blob
`b41b4183b6a7f8f795a3e985acf94a364dc435e7`. Its XML is not redistributed.
The 600-nit boundary and 4095-to-2047 conversion retain the existing native
normal range and visible minimum 21. The paired KTZ8866 LUT stays in the kernel
and is not applied twice. This does not enable HBM or reproduce all Android
scene, history, filtering and learning policy.

The startup integration uses original Plasma's `runEnvironmentScripts()` before
`startPlasmaSession()` in plasma-workspace commit
`3b302fa3f81cafc97e8376523ce5eddfd560a0bd`, `startkde/startplasma.cpp`, blob
`9cda0be1e540f257b09ef81b9a7a3707bc869dba`. KWin's configuration is loaded at
startup; rewriting its active file alone is not a live curve API.

Implementation, native tests, RPM ownership transition, actual session import,
optical comfort, broad lux response and sensor power acceptance are separate.
No physical or package-delivery success is claimed by this source draft.
