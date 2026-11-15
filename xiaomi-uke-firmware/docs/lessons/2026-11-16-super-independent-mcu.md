# UKE-MCU-SUPER-20261116: retain firmware before repurposing super

Date: 16 November 2026. Environment: Fedora Rawhide on POCO Pad X1,
kernel 7.3.3-28-unstable; separate PC source/data investigation.

The managed firmware helper currently obtains the keyboard controller image
from stock ODM/vendor inside super. Replacing that container with swap and
rescue would remove the acquisition source for later kernel transactions.

The Uke-only public vendor repository omits the image. The shared-IP vendor
repository contains it at commit d4bf71b32fc8e82fbe6ee2dcbbdd4272f1036154,
path proprietary/odm/firmware/MCU_Upgrade.bin, blob cec99c93b6593659387eda71ea34be96b547d3ea.
The 82,740-byte image hashes to b17fa938cf5613dfa550179d6fc40cfbaa831d534a16a22282e2904e845fa7c4,
exactly the existing whole-file Uke admission. A shared-SoC repository name is
not compatibility evidence; the independently pinned complete identity is.

Firmware 17 adds this generic image and provenance to the existing package.
It retains all 16 inputs, OEM notices, original file-count and offline audio
tests through a complete recipe inverse. No private calibration, new daemon,
controller transport or flash operation is introduced. The original proprietary
classification and incomplete file-level redistribution review remain explicit;
the source repository has no top-level license.

The managed helper must still qualify a protected, bounded, exact-hash file
source before super changes. This data RPM alone does not migrate that reader,
prove keyboard functionality or authorize unqualified super writes. Existing
CDSP/authentication files remain read-only device-local sources in modem.

Reference: [pinned public MCU directory](https://github.com/Xiaomi-Pad-7-Pro-Resources/android_vendor_xiaomi_sm8635-common/tree/d4bf71b32fc8e82fbe6ee2dcbbdd4272f1036154/proprietary/odm/firmware).
