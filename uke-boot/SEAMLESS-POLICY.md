# Seamless updates and local rollback preference

4 November 2026. Seamless package updates remain enabled independently of the
rollback mode. The kernel updater RPM installs automatic=1 as its default and
enables both admitted automatic-policy gates during upgrade and reinstall.
Existing display policy is retained. This is a settings migration; it does not
itself choose a different kernel profile or request a reboot.

Rollback defaults to enforced for new installations. A user chooses off with
`sudo uke-control set rollback-off`. The root-owned rollback.conf is an RPM
noreplace configuration file: updates and reinstalls keep off without repeating
that command. The seamless migration never changes the local rollback file.
No package payload or scriptlet selects off for users.

Off skips health acceptance and automatic fallback. It keeps bundle validation
and seamless staging active. A recorded booted unstable A trial can receive a
newer selected unstable profile through the retained checked replacement
transaction, preserving B, its flags and protected baseline. The explicit local
off setting authorizes that development policy; enforced/permissive retain the
ordinary acceptance path. Failed releases are not automatically retried, older
development profiles are not automatically restaged, and an already pending
trial waits for its reboot. No update fabricates healthy acceptance.

The producer retains the published 1.8.60 source RPM by SHA-256. Its existing
transaction boundary, partial-write restoration and sanitizer fixtures remain
in place. New COPR fixtures cover repeated policy application with all three
local rollback modes, preserved display preference, malformed/symlink refusal,
automatic stage decisions, pending boot identity, failed/downgrade guards and
off-mode acceptance isolation. Physical validation remains separate.
