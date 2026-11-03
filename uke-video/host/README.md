# Retained video encoder copy correction

3 November 2026. Build this recipe only in the existing
`mcc45tr/uke-linux-unstable` project, `fedora-rawhide-aarch64`.

`copr-retained-encoder-copy.sh` verifies the signed integration from COPR build
11239487 and keeps its original source archive, runtime dependencies, lifecycle
scripts and native checks. The additional MIT patch selects the existing CPU
fallback for the encoder's private staging copier. Decoder policy, direct
imports, buffer lifetime and hardware encoding are preserved.

The source revision advances to `uke-video-integration-1.0.0-2.fc46`; the backend
becomes `uke-vaapi-v4l2-1.0.0-12.fc46`. The unchanged session source receives
`uke-video-session-1.1.0-3.fc46` so an existing binary identity is not reused.
The one-time custom source worker must check out an immutable reviewed Git
commit and call this script with its result directory. Do not run a build on
the tablet or install a copied driver. Deliver signed results through DNF.

Native COPR checks and installed image-quality, decode, driver-selection and
lifecycle tests remain separate. No stable promotion or Core dependency change
is implied by source preparation or a successful build.
