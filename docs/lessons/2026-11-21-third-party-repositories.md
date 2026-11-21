# Third-party repositories on Fedora Rawhide AArch64

Lesson ID UKE-THIRDPARTY-20261121
Date 21 November 2026
Environment Fedora Rawhide AArch64 on POCO Pad X1, Linux 7.3.5-44

Google's ARM64 RPM metadata lists Chrome Stable, Beta, Dev and Canary in one
repository. Each RPM's installation script can create a separate repository
file pointing to that same URL. The Chrome configuration subpackage registers
one package-scoped repository and supplies RPM-managed defaults for each channel.
The four Chrome installs left one Chrome repository on the tested image.

SolidCraft upstream publishes an unsigned RPM and a release checksum. The
production COPR rejected direct binary upload. The source worker instead verifies
the upstream SHA-256 and unpacks the published AArch64 RPM in the binary build.
All 18 installed file digests matched upstream. COPR signed the resulting RPM
and published it in a separate directory in Uke Unstable. [Upstream release](https://github.com/tangency-labs/solidcraft/releases/tag/v0.45.0) · [COPR SolidCraft build](https://copr.fedorainfracloud.org/coprs/build/11372045/)

The repository configuration must be installed before DNF resolves the
application package names. The signed configuration RPM and its two
subpackages use build 11372043. The tablet used a package-scoped Unstable
repository for the first transaction because its installed channel was Stable.
[Configuration build](https://copr.fedorainfracloud.org/coprs/build/11372043/)

An independent reboot interrupted the initial Chrome transaction. The next
boot retained the configuration packages. DNF finished installing all four
channels, which each rendered a local HTML document in headless mode with
the browser sandbox enabled.
The interrupted transaction had installed the Liberation Fonts metapackage.
Installing its three matching Fedora font packages restored dependency closure.
The final DNF dependency check and installed RPM verification passed.

SolidCraft 0.45.0 measured the expected 1,000 mm³ volume of a test solid and
exported STEP. It selected the Turnip Vulkan adapter for Adreno 732 and
produced a GPU-rendered PNG. The application tests exercised local browser and
CAD execution. A graphical desktop session was outside the smoke tests.
