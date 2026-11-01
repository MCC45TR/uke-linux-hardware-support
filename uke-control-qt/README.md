# Uke Control for Qt

A native C++20 Qt 6 Widgets application for the existing Uke Control client.
It uses the distribution's original Breeze style plugin when installed. It has
no QML, Python, WebView, replacement desktop service, or privileged GUI.

The daily pages are Status, Power, Display & lights, Connections, Audio & sensors,
and Updates. Advanced provides searchable access to the complete installed action
catalog. Opening a page or guide does not query devices. Select **Read observation**
or an inspection button to read. Requested defaults are drafts rather than current
measurements. **Details** reveals the full catalog explanation, parameter limits,
action identifier, access requirements and, during review, exact validated arguments.
Maintenance and external-publication warnings remain visible before confirmation.

## Build and run

Install a C++ compiler, CMake, Qt 6 Widgets, and the existing Uke Control package
providing `pkgconfig(uke-control-client)`, `client.hpp` and `frontend.hpp`.
On Fedora, install `qt6-qtbase-devel`, `plasma-breeze-qt6`, and the original
`konsole` for authenticated foreground operations. The source-tree build also
finds the sibling `uke-control/build/libuke-control-client.so.1` when pkg-config
is unavailable.
Repository integration can explicitly select the current source SDK with
`-DUKE_CONTROL_SOURCE=/path/to/uke-control`; this takes precedence over the
installed headers and library. Standalone package builds use pkg-config.

```sh
cmake -S uke-control-qt -B uke-control-qt/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build uke-control-qt/build -j4
uke-control-qt/build/uke-control-qt
uke-control-qt/build/uke-control-qt --read-only --start battery.status
```

Run as the normal graphical session user. A root GUI is refused. `--help`,
`--version`, and strictly isolated fixture sessions are exceptions. If Breeze
is absent, a visible footer identifies the installed fallback style and the
missing package; the application does not imitate Breeze with a custom stylesheet.

## Reads and reviewed changes

Captured observations use the shared read-only `Query` on the Qt event loop.
Stop, Back, changing pages and closing the window cancel the owned read. Each
query retains its backend timeout and bounded stdout/stderr. Refresh only repeats
the last validated read; it cannot replay a mutation. Structured values, original
bounded output and errors have separate tabs. Null, false, zero, negative values,
unavailable providers, cancellation, timeout and truncation remain distinct.
Control and directional override characters are rendered visibly. Large structured
views are bounded to 2,000 values and 16 levels; the raw bounded result remains
available.

Changes pass shared typed validation and open a separate native review dialog.
The requested values are visible before continuing. Package operations require
`APPLY`, maintenance requires its action ID, and publication requires its exact
repository, in addition to separate consent. Read-only sessions block changes.
The shared `frontend.hpp` adapter revalidates the ID and values, then opens
an original distribution terminal. Its small, unprivileged native
`frontend-request` wrapper revalidates the request and uses the existing
`run_terminal` path, with `sudo --` only when required. It uses no shell.
Authentication and all existing backend admission and interactive confirmations
remain in that terminal. The wrapper displays the exit status and waits for
Enter, preserving the result for inspection. Opening a
terminal establishes neither command completion nor hardware success. The GUI
never retries writes, refreshes them, or interrupts an independently handed-off
terminal operation when it closes.

## Simulation and tests

```sh
uke-control-qt/build/uke-control-qt \
  --fixture uke-control/tests/fixtures/tui.json --start charge.mode
ctest --test-dir uke-control-qt/build --output-on-failure
```

Fixture mode shows a persistent **SIMULATION** indicator and blocks every real
query, command, terminal handoff, mutation and receipt save. It accepts the existing
`org.uke.control.tui-fixture.v1` schema from a non-symlink regular file of at most
1 MiB. Results use known action IDs with `data` or `stdout`, optional `stderr`,
exit status 0–255 and delay 0–3,000 ms. Unknown fields and incompatible fields
are rejected. Missing results stay unavailable and never fall through to a real
query. No executable callbacks are supported.

The C++ Qt tests exercise typed values, separate errors, read refresh, unavailable
fixtures, cancellation, page/search/form navigation, native approval requirements,
read-only and simulation blocking, and close cleanup. The shell test checks CLI
help/version, invalid fixtures and root refusal. Tests use the sibling TUI fixture;
source archives built against an installed client may disable tests with
`-DBUILD_TESTING=OFF`.

## Packaging and observed validation

`packaging/uke-control-qt.spec` builds the independent `uke-control-qt` RPM against
the existing monolithic `uke-control` package. CMake installs the executable,
desktop entry and scalable icon using standard GNU installation directories.
It does not change the existing family package metadata.

On 1 November 2026, the Fedora 46/rawhide software container built the application
with GCC 16.2.1 and Qt 6.12.0. The installed native Breeze plugin was
`plasma-breeze-qt6-6.8.2-1.fc46`. Native Xvfb screenshots were inspected at
1120×800 and 780×720, including charging preferences, simulated observations,
review blocking and an actual nonroot charging read. The actual read returned
`available: false`; this was displayed as provider unavailable. A staged CMake
installation was executed against the installed shared client.

Screenshots and logs stay under ignored `build/`. These are source, build and
synthetic desktop checks. No tablet, physical touch acceptance, charging cutoff,
display receiver, storage mutation or package transaction was tested.
