# Retained KDE dictionary correction

3 November 2026: this COPR source recipe selects only the existing
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
