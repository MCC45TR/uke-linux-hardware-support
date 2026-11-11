# Deliver the qualified charge-helper wait

11 November 2026. Control 2.17.0-2 is a prepared packaging candidate over the
actual signed 2.17.0-1 source. Its wait change passed AArch64 qualification in
COPR 11304610 at source `088a1ed3a950ee4c04b307fc0405ec5a1ebb62f8`.
The runtime update is not yet built, installed or measured on the tablet.

Keep all 51 parent source files unchanged and add one source overlay. Apply
only the qualified helper patch after the existing charge overlay has checked
its original inputs. Reverse the complete runtime source tree to prove that
hardware and control bodies are retained. Change the RPM Release consistently
across the same three existing package names and their versioned dependencies;
retain every original check, scriptlet, configuration and service definition.

The package build reruns the actual wait/control/I/O and signal-error fixtures
with GCC and Clang sanitizers. It does not execute device-facing main or a
firmware operation. Node.js is declared as a COPR-only test build dependency;
no runtime dependency, service, runtime RPM or debug package is added.
Keep the periodic guard and its two-second protection interval until
native event/action/lifecycle parity is independently accepted.

Before installation, verify signatures, dependency and payload closure, normal
DNF upgrade/fresh/reinstall/rollback resolution, persistent settings and unit
states. Compare scheduler activity passively after the existing helper restarts
through its supported package lifecycle. Context switches and CPU runtime are
not hardware wakeup counts or electrical energy measurements.
