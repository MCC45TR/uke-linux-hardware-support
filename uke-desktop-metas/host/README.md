# Retained KDE dictionary correction

3 November 2026: COPR build 11246994 passed the retained XML, profile and
native QML/cache checks on Fedora Rawhide AArch64. It produced only
`uke-kde-plasma-meta-1.0.0-35.1.fc46`, plus the source RPM. Signed package
inspection confirms unchanged KDE file contents, ownership, permissions and
lifecycle scripts; the added runtime requirement is `hunspell-tr`.
The exact DNF update installed the pair on Fedora 46, and the original
Hunspell command accepted Turkish words and rejected an invalid example.
The existing keyboard process was not restarted, so its next dictionary
load remains separate from that command-line test.

This COPR source recipe selects only the existing
`uke-kde-plasma-meta` package from signed Control source build 11240837.
Its source release is `2.8.1-5.1.fc46`; the sole binary release is
`1.0.0-35.1.fc46`, between the installed selection and the separate SDK cohort.
The recipe adds original Fedora `hunspell-tr`, preserves the KDE payload and
lifecycle sections, and runs the retained metadata checks before pruning
unselected buildroot files. It produces no backend, SDK or power package.

Run the source recipe only in COPR `mcc45tr/uke-linux-unstable`, chroot
`fedora-rawhide-aarch64`, with an immutable reviewed Git revision. A one-time
custom build can use this recipe without replacing another package producer.
Source validation does not establish a successful native build, installation
or physical keyboard result.

The tested source commit is `b98f24ac42e444fa956baaab889f6c256eb15fe3`.
These NVRs are reserved. Future changed artifacts must advance both releases.
The successful build retained a nonblocking missing-changelog epoch warning;
a future source revision should preserve or add a changelog entry.
