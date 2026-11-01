# Tailscale resolver publication source

1 November 2026: this manual Fedora46 AArch64 source candidate labels the
completed resolver temporary before its atomic rename. It retains the original
Fedora1.104.4 vendor archive, existing post-rename handling, backup/container
fallback, unit and preferences. It adds no SELinux permission or runtime watcher.
The source and upstream BSD-3-Clause attribution remain in the complete SRPM.

Fedora's default resolver policy must give its temporary and destination paths
the same context. The native build checks that contract. Custom file-context
changes need independent validation. Remote fixtures use synthetic user xattrs
and fake external commands to observe publication ordering; they do not prove
real SELinux or tablet network behavior. The original-source negative control
must fail for the missing pre-publication context. Fedora's broad vendored test
suite remains excluded because its complete test dependencies are unavailable.

The existing automatic runtime catalog and boot producer are unchanged. Release
1.1 sorts after Fedora1 and before Fedora2. With local vendor changes disabled,
installation and a future return to Fedora require transaction-local vendor
permission. The package retains truthful COPR vendor metadata; it does not alter
global DNF policy or claim automatic security-update maintenance. Original Fedora
upgrade scriptlets restart tailscaled, so delivery needs live-network checks.
