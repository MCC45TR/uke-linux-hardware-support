# USB-C source candidate

Date: **18 October 2026**. Target: Xiaomi Pad 7 / POCO Pad X1, SM7675.
Environment: source editing on Fedora. No test, compilation, package build,
device operation or publication has been performed in this USB conversation.
Coordinator-reported host results apply only to their exact recorded revision.

`uke-usb-port` reads cached Type-C, USB-role, UDC and external DP state.
It delegates power-supply collection to the charging provider: even a read-only
power-supply getter can issue firmware GETs. Missing information is `null`. It neither
loads a driver nor activates UPower, NetworkManager or another D-Bus service.
`--root EXTRACTED_ROOT` reads an offline fixture. The optional
`uke-usb-observer.service` exposes `GetState()` on `org.senemos.Uke.USB1` at
`/org/senemos/Uke/USB1`. Installing the package does not start it. There are no
raw hardware write methods and no KDE application is replaced. The paired
development-image composer explicitly enables the cached-state observer. Installing the
package alone still starts no service.

Class attributes are sequential observations, not a coherent hardware snapshot.
They do not establish a physical connector-to-controller association, current
attach generation, applied role, DP training, effective charger control or an
electrical shutdown. It reports `DELEGATED_TO_CHARGING_PROVIDER` instead of
opening power-supply attributes. The charging provider's eventual contract and
measurement provenance must remain explicit. USB serial strings and EDID
payloads are excluded.

The shared C++ policy source keeps data role, power role, orientation and lane
allocation separate. It rejects stale observations and voluntary disruptions of
active storage/network use. Host with sink power is a valid target. The planner
does not apply hardware changes: source, power-swap and lane backends still need
their exact qualified providers. DP budget arithmetic uses trained lane/rate,
retains HBR2, and does not promise a resolution or refresh rate.

On the actual Uke root, the observer also uses libdrm's cached connector query
on read-only MSM card descriptors. It never requests DRM master, forces a
connector probe or submits a modeset. It reports cached modes with complete pixel
clocks, active CRTCs and positions, and the requested `max bpc` property when
present. A requested value is not a measured wire depth. Distinct CRTCs do not
prove an extended desktop; the visible result and compositor layout need their
own check. Raw EDID and serial strings remain excluded. Offline fixtures never
open the host's DRM devices. The cached WCD revision attribute added by the
matching kernel is reported separately; missing data stays null.

The requested external-screen target is an extended desktop at 75 Hz with RGB
10-bpc output. Until the monitor is identified, 2560x1440 is only the assumed
meaning of "2K". The planner uses the complete EDID timing clock and keeps the
requested depth. It can propose DP2+USB3 or DP4+USB2; it refuses a voluntary
DP4 change during active USB I/O. At an illustrative 304250-kHz clock, RGB10
requires 9.1275 Gbit/s, exceeding two HBR2 lanes' 8.64 Gbit/s payload. This is
arithmetic, not the monitor's EDID, trained capacity or a successful modeset.
Audio/transport overhead and atomic DRM validation remain final constraints.

## Explicit debug gadget ownership

The existing `senemos.debug=cdc-acm` development profile retains CDC and optional
ACM+ECM support. Both paths use the same bounded lock. The configfs helper resolves
the one admitted UDC from the physical provider ancestry and fixed peripheral
FDT; it no longer binds a hardcoded controller name. It prepares descriptors and
both functions before publishing the gadget. `MaxPower` is 500 **mA** in configfs,
matching the configured USB2 budget; it does not authorize a new PD contract or
guarantee measured current.

A root-owned runtime inode record identifies objects created by this helper.
Unknown or partially foreign gadget trees are refused. Cleanup removes only the
known links/functions and does not recursively delete another configuration.
An unrecorded gadget from an older helper requires a controlled legacy teardown;
the new helper does not adopt it by matching a product string.

Before a future policy owner stops the debug units for a planned transition, it
must create `/run/senemos-uke/usb-gadget-inhibit`. Legacy start/fallback and shell
units then refuse to reopen the profile. An optional saved
`/etc/senemos/usb-sharing` value other than `legacy-debug` also inhibits legacy
export. These guards do not disable CC/PD, VBUS or battery charging. No new
automatic role owner or sharing-preference writer is shipped here. Existing
NetworkManager local-only addressing remains in its original profile.

The paired initramfs composer includes the helper and its exact command
dependencies. Existing switch-root service conflicts still stop early units.
The historical frozen boot-pair identity is unchanged; a future candidate needs
a newly generated matched boot/root tuple. Updating packages alone is not that
handoff and does not flash a boot partition.

## Automatic PC-attached CDC candidate

`uke-usb-gadget-ownership-api = 2` adds `uke-usb-auto.service` and the explicit
saved policy `auto-debug`. With `senemos.debug=cdc-acm`, it reconciles every two
seconds and offers ACM only after observing a PC/device-role attachment. An
admitted graph requires exactly one matching USB-role provider reporting
`device` and a corresponding Type-C partner with device data role. The former
SDP/CDP power-supply fallback is removed: the charging source owner confirms
that no qualified cached attachment API exists. A host/none role, unknown attachment,
inhibit file or ambiguous provider does not admit export. Type-C/data/power roles
are never written by this helper. The reason is recorded privately under `/run`
and exposed by the observer.

The watcher and legacy ACM/network units conflict. All configfs actions share
the existing lock and inode ownership check; the watcher removes only its own
`auto-acm` profile. It does not unload an already inherited `g_serial` owner,
start a root shell or export ECM, files, cameras or microphones. A new matched
boot/root pair must perform the early-to-real-root ownership handoff. The early
fixed-profile rescue console is retained: automatic PC detection before remote
firmware is available is not established by this real-root candidate. CDC log
forwarding to an ESP32 is a separate service and is unchanged here.

The fixed peripheral producer therefore retains guarded legacy ACM instead of
enabling a watcher that cannot establish PC attachment. Select `auto-debug` and
enable the watcher only in a matched image with the actual Type-C/role graph.
Transport diagnostics or a model-attestation file do not grant that admission.

The source fixtures cover actual attachment-decision functions, disabled and
ambiguous states, host/none rejection, absence of power-supply reads and the
QHD/depth budget. They are written but have not been run in this thread. Service lifecycle,
SELinux permissions, real configfs cleanup and physical CDC remain untested.

## Remaining source and acceptance work

The current fixed peripheral DT and DWC3/eUSB2 host gates remain in place.
Full host requires a qualified WCD route/EQ and firmware-owned VBUS profile.
The kernel candidate now supplies the QMP normal tuning, explicit DP match,
DP0 DPU interface and DP clock voltage-vote sources. Its provider graph is still
incomplete. The native display profile disables OEM `sde_dp` and leaves DP clock
inputs unconnected; AUX/HPD/connector/codec integration remains source work. Passive analog
audio also needs the actual switch/codec/jack/microphone graph. Class-function
Kconfig settings do not supply MTP/ADB/UAC/UVC userspace servers or permission.

No host, SuperSpeed, DP, reverse-charge, analog-audio or dock success is claimed.
`make test-usb`, kernel protocol KUnit and joint accessory trials are deferred,
not passed. Source changes must be reviewed and included in the source archive
before building a new matched tuple; the release producer archives committed
sources. Physical support remains the status of the previously recorded tuple.
