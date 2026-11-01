# Qt Virtual Keyboard sound component

1 November 2026. Original Fedora Rawhide `qt6-qtvirtualkeyboard-6.12.0-1`
omits the Qt Multimedia development dependency. Upstream installs
`MultiSoundEffect.qml` only when that dependency enables its sound feature,
while the keyboard attempts to create that type. POCO Pad X1 logs record the
resulting missing-type errors. Installed Qt libraries and files match that
Fedora version; adding the runtime Multimedia package alone cannot change
the build feature.

The manual source recipe retains the complete signed-source-verified Fedora
input and unchanged Qt sources, enables the existing feature, and adds matching
Qt development dependencies. Release `1.1` permits a later Fedora Release `2`
to replace this build. Original KDE applications and the existing automatic
runtime catalog are unchanged. The source lock is
[`qt-virtualkeyboard-sound.json`](../../manifests/qt-virtualkeyboard-sound.json).

COPR must compile and instantiate the packaged QML type offscreen, including a
missing-type control, without calling audio playback. Signed RPM admission and
an unstable-only DNF dependency preview precede installation. Actual keyboard,
audio, startup and shutdown acceptance remain separate physical gates.
