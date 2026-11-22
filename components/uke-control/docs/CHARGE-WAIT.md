# Interruptible charge-helper deadlines

11 November 2026. This source work retains the protection helper's two-second
guard interval and the admitted automatic controller's lease-based interval.
It replaces their100ms sleep loops with one deadline wait when uninterrupted.
It does not retire the guard or claim complete hardware event coverage.

The helper blocks SIGTERM/SIGINT across the stop-flag check, then atomically
restores the caller's signal mask inside `ppoll`. Pending stop signals can
therefore end the wait without a lost-signal window. A non-stop interruption
reuses the original monotonic deadline. Other signal-mask settings are restored
on success and syscall error; failures remain explicit errors. No new thread,
file descriptor, service, library dependency or runtime package is needed.

Only waiting changes. Device reads, writer admission, wet/fault handling,
deadlines, service configuration and user settings remain unchanged. All
required bridges stay in the existing Control package. The original legacy
guard's hardware coverage is retained until a separately qualified native
event/action path can replace it.

COPR qualification must exercise the actual helper function with a real
two-second idle wait, pending and delayed stop signals, an interrupted deadline,
signal-mask retention and injected syscall errors. The full ARM64 helper must
also compile. No test executes the device-facing helper main or performs a
firmware operation. Compilation, RPM delivery, target wakeup measurement and
physical charging remain separate acceptance steps.
