# UKE-CONTROL-UI-20261101: separate daily settings from expert tools

Date: 1 November 2026.
Environment: disposable Fedora Rawhide x86_64 container, GCC 16.2.1; ncursesw,
GTK 4.24.1/libadwaita 1.10.0, Qt 6.12.0/Breeze 6.8.2; Xvfb and native PTYs.
No tablet or host device was accessed. This is source and host UI evidence.

## Finding and implementation

The original terminal home exposed ten equal categories and numerous technical
observations next to daily controls. The catalog already provided a suitable
shared boundary: `client.hpp`, `libuke-control-client.so.1`, typed plans and
single-owner asynchronous captured reads. Replacing that boundary or inventing
new privileged methods would duplicate backend policy.

The terminal now has seven daily areas. Its explicit daily allowlist routes new
catalog additions to expert areas until reviewed. All 126 action IDs remain
reachable. Captured parameter-free daily observations open directly; writes keep
guides, review and original backend authentication. Read refresh replaces the
result page and preserves its earlier receipt, avoiding an ever-growing back
stack. Foreground commands and mutations are never replayed by refresh.

Two independent C++ applications use GTK4/libadwaita and Qt6 Widgets/Breeze.
They reuse the same client catalog, parameter validation and Query ownership.
`frontend.hpp` provides a shared, header-only terminal handoff that revalidates
an action ID and typed values. A small nonroot terminal companion revalidates
the request, uses the existing `run_terminal` per-action authentication path and
keeps the actual exit result visible until Enter, avoiding disappearing error
output. No raw command, shell, new D-Bus method, blanket
Polkit policy or alternate storage writer is added. Native review remains
separate from terminal authentication and the backend's own confirmations.
Launching a terminal does not establish that a request completed. GUI closure
does not terminate foreground maintenance.

## Validation and corrections

`make -C uke-control all`, targeted client/TUI/bus/layout checks and repository
source validation run in the isolated container. PTY/libvterm tests cover all
126 placements, daily/advanced separation, immediate reads, refresh/back,
error/empty results, maintenance confirmation refusal, keyboard/touch, dragging,
forms, Unicode, search, history, cancellation, resize and terminal restoration.
The installed TUI is also exercised through PTY and an Xvfb/xterm window.
Redirected input fails cleanly without terminal control output.

A real unprivileged installed `battery.status` query returns status 69 and
`install_uke_battery_observer_for_battery_reports`. The TUI displays unavailable
state and that reason. GUI charge queries return `available:false` with an empty
threshold list in this provider-free container. Those are actual missing-provider
observations, not simulated successes. Numeric battery/charging results and
control-form walkthroughs use visibly labeled fixtures; fixture mode starts no
commands, writes or terminal handoffs.

Early terminal screenshots used a non-UTF-8 xterm and synthetic key events before
focus; these produced mojibake and incomplete captures. Corrected evidence uses
`xterm -u8`, C.UTF-8, focused XTEST input and settled rendering. Native GUI visual
review also checks narrow windows and long choice explanations.

The broader `make -C uke-control check` reaches the unchanged package fixture and
fails because the downloaded Rawhide container identifies itself as
`VERSION="46 (Container Image Prerelease)"`. The unchanged `fedora-target.hpp`
accepts the repository's explicit numbered releases or recognized Rawhide names,
and intentionally refuses this spelling. No release-admission rule was relaxed
to obtain a passing UI test. This remains a separate backend/environment issue;
recheck the full package suite on an admitted Fedora target.

The source-family offline split/filelist/cohort/service/contracts/tracking checks
pass. The subsequent unchanged external Fedora-return ABI audit stops at HTTP 404
for its pinned `uke-atk-2.62.1-17.fc46.aarch64.rpm` in archived COPR build 11228248.
The public key remains available (HTTP 200); the failure is the archived RPM, not
the key. This UI work neither changes that pin nor publishes a package.
A temporary terminal stub in the disposable container
also captures the exact fixed companion and typed `charge.mode mode=firmware`
arguments. It executes no backend and is removed immediately. This proves
argument handoff only, not real desktop terminal authentication or a device write.
The companion is also compiled with the existing test-only backend executable
selected at build time. Native terminal runs retain its synthetic exit 7 and
interrupted exit 130 until Enter closes the window. Production binaries have no
runtime backend override.

`make check-guis` passes the GTK fixture parser and both Qt suites. The GTK
installed-binary Xvfb smoke separately exercises navigation, cancellation, native
review and the simulation APPLY gate. Both independent RPM builds pass their
embedded checks and desktop validation; extracted RPM executables run nonroot
under Xvfb. These laboratory RPMs use `--nodeps` because the current SDK was
manually installed, not registered in the RPM database. They are x86_64 host
artifacts, not tablet packages. The core source-RPM target also completes.

## Consequence and next validation

These source previews remain outside the published source-family revisions,
COPR publication and Fedora r2 image inputs. Verify the installed GUI packages in
real GNOME and KDE sessions, exercise existing authenticated terminal handoff,
and perform separately authorized hardware acceptance before claiming charging,
screen, microphone, touch or recovery support. Xvfb, fixture values and command
exit statuses do not establish physical behavior.

## Review follow-up — 1 November 2026

The package-fixture failure above was narrowed to an optional Rawhide-only test
branch calling the real runtime release parser without handling an unsupported
build host. The test now explicitly skips that branch on an unsupported host,
continues all other package checks and asserts that the Container Image
Prerelease spelling remains refused. Neither `src/fedora-target.hpp` nor runtime
`src/packages.cpp` changed. This does not qualify that container for channel
switching; the optional configuration branch still needs an admitted Rawhide host.

The unchanged ABI audit and complete source-family aggregate check subsequently
pass after populating their existing ignored cache with exact archived artifacts.
The existing GitHub release `copr-consolidation-archive-20261101` contains all 13
fork RPMs; each release digest and independently downloaded SHA-256 matches the
pinned manifest. The missing uke-atk file is 84,895 bytes with SHA-256
`c056d680bac275946f71feeb417e3963aa25d71a5240913e6f81f9896468d949`.
The direct COPR URL remains HTTP 404. The audit separately verifies all 13 signed
pairs, 13 DSOs, 219 interfaces and 836 data files; payloads are not executed and
replacement admission remains false. No pin or runtime dependency upgrade was
used to obtain these passes.

Both GUI specs now explicitly require the installed `frontend-request` path and
build against the installed `frontend.hpp` path, in addition to the existing
client/version requirements. This prevents an older nominally matching core
package lacking the new companion from satisfying the GUI contract.

Remote CI verification for source commits `59f26ba` and `a190182` found zero
Actions runs. `a190182` has zero check runs and zero legacy statuses; GitHub's
combined status is `pending` because no statuses exist. These commits deliberately
use `[skip ci]` to avoid a COPR request. Local test results are not remote CI passes.

After the test-only correction, `make -C uke-control check` passes completely in
this laboratory, with the optional Rawhide-specific configuration branch
explicitly skipped. `make validate` also passes. A normal core RPM build passes
build dependency admission and package assembly without `--nodeps`; its package
owns the required client library, SDK header and terminal companion.

Normal DNF dry runs of both rebuilt GUI RPMs correctly refuse default Fedora
repositories lacking Uke Control. Including the actual locally built core RPM
satisfies their new core/file/library requirements, but the transaction still
cannot resolve the core's existing `uke-kernel-update >= 1.8.7` and
`uke-kernel-update(transaction-v1) >= 1` dependencies from the enabled repositories.
No provider RPM is available in this workspace. No missing dependency was
stubbed, bypassed, downgraded or installed, and no unrelated repository or package
upgrade was used. The GUI RPM assembly tests are passes; complete normal package
transaction acceptance remains blocked on that existing external core dependency.
