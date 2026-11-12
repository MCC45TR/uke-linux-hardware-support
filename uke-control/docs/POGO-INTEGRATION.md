# Pogo integration in Control

12 November 2026. Candidate Control 2.17.0-14 folds the accepted pogo-auth4
and pogo-desktop5 sources into the existing Control RPM. It creates no runtime
subpackage, firmware payload, desktop fork or unconditional authentication unit.
Build, signed delivery and physical acceptance are separate gates.

The original kernel bridge owns accessory transport, attachment, challenge
epochs, token admission, input and the conditional keyboard LED. The retained
client still requires exact Uke/SM7675, the bound NanoSIC driver, QTEE object
references and the locally provisioned exact TA. Protocol, token errors and
the four-second userspace budget are unchanged. Secure-world
execution and physical keyboard authentication remain unqualified.

The release check now accepts the published 7.3.1 and 7.3.3 unstable families,
with a positive numeric revision. Other families, stable releases and malformed
paths remain refused. This corrects the existing helper's rejection of the
running 7.3.3 family without relaxing board, protocol or firmware admission.
Healthy authentication idle blocks on the kernel request or a stop event.
A lock-free signal handler writes a nonblocking eventfd, closing the signal
before poll race without a periodic timeout or fixed sleep.

The original unprivileged Qt helper consumes original UPower and PowerDevil
interfaces. Its C++ source is unchanged. KDE autostart remains KDE-only; it
never activates a stopped PowerDevil. Its runtime libraries are conditional on
PowerDevil, so console and GNOME profiles do not acquire a KDE runtime. The
system authentication condition uses the native auth executable and the exact
same attachment policy header, without Qt or a TEE operation. Root activation
still requires the real conditional LED event, attachment and firmware gates.
Existing unit masks retain precedence. DNF weak-dependency settings are not
modified.

Authentication now links the original Fedora quic-teec shared library instead
of embedding QCOMTEE and QCBOR. The signed Fedora 1.0.2-3 source RPM SHA256 is
291fcd39de0c04cad343ff33195f62d00f12ce7d7ab3d9d4765b7dd1ee4e0021.
All three public headers and the object/memory implementations match the
accepted Qualcomm source pin. Fedora applies build and SONAME patches and
uses its original libcbor provider. A native fixture calls that installed
library's real credentials callback, decodes its UID/time map, exercises
invalid offsets and operations, and verifies no TEE access. This is a software
callback gate, not secure-world or memory-allocation failure acceptance.
The retained QCBOR error-injection fixture applies only to the old test source;
neither that patched encoder nor QCBOR is linked into the new runtime.

The unchanged MINK adapter remains embedded because Fedora currently offers
its IDL compiler, not the adapter library or MinkCom interface. It translates
the existing Object ABI to the distribution QCOMTEE API; no Qualcomm client
fork is published. The return path is an original distribution MINK adapter
or a separately qualified direct QCOMTEE client, preserving every object,
callback, lifetime and error path. OP-TEE's GlobalPlatform client cannot
replace this QTEE object-reference protocol.

RPM pre/posttrans snapshots and restores known system/user unit states. An
active helper refreshes after ownership changes; an inactive, masked or
currently stopping helper is not started. A logged-out session is not started
by the transaction. Failed restoration retains its protected receipt. Ordinary
Control upgrades and reinstalls also refresh existing providers. No hardware
state, credential, calibration or private value enters that receipt.

The accepted source RPMs and signed standalone runtimes remain in the
[recovery archive](https://github.com/MCC45TR/uke-linux-hardware-support/releases/tag/copr-consolidation-archive-20261101).
Their complete source inventories and the original MINK/QCOMTEE/QCBOR pins
are sealed in pogo-services.json. BSD/MIT notices and the Linaro interface
notice remain packaged. Original protocol/credential and desktop state tests,
new event/stop tests and every Control12 native gate are retained. Native
results, normal DNF/fresh/reinstall/rollback, session startup and unavailable
physical attachment/input/light/suspend tests must be recorded independently.

A review of the first Control13 candidate found that its root auth unit had
no system-unit removal hook, matching the standalone auth4's old omission.
Control14 adds the standard systemd install/remove/reload macros alongside
the user-unit hooks. Removing Control must stop the root helper; ordinary
upgrades still restore its captured state. Candidate13 is not delivered or
used to retire either standalone producer.

Control14 also snapshots in preun when an upgrade has no earlier receipt and
restores in postun when another package instance remains. This covers returning
to accepted Control12 plus the signed standalone helpers, whose old install
scripts do not refresh running helpers. Mixed ownership during an RPM
replacement accepts only the two known standalone names and Control. An
earlier pre-transaction receipt is never overwritten. Full removal skips
restoration and uses the normal system/user unit removal hooks; it cannot
start either helper. The same data fixture exercises the mixed-owner rollback,
preexisting receipt and full-removal paths.
