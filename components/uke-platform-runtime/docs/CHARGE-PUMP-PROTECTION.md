# Charge-pump and integrated-protection coverage

29 October 2026. POCO Pad X1 source review; Fedora Rawhide target. No tablet
access, firmware request, bus scan, register write, installation or physical test.
This inventory and admission requirement is not an activation receipt.

## Ownership and provenance

Exact OEM DT routes battery control through ADSP `charger_pd` and PMIC GLINK.
The shared Xiaomi wrapper exposes CP_MODE, BQ2597X and SMB1396 names; those do
not identify fitted chips, two pumps or cell topology. No AP pump binding was
found in the reviewed Uke board fragment. No direct pump driver, ratio/bypass
setter, protection-register writer or second watchdog feeder is enabled.

The DFX message route names CP bus/battery OVP/OCP, die temperature and bus
failures, but `xm_handle_adsp_dfx_report()` only bounds-checks the type and
returns. That empty AP handler proves neither firmware protection nor its
absence. Native Linux has no qualified equivalent decoder.

| Source | Exact reviewed revision and files |
|---|---|
| MiCode Uke kernel | `5712080a4b0c0b1a879e6d145c601c649ddaddfd`; `drivers/power/supply/qti_battery_charger.c`, `charger_partition.c`, `bq25980_charger.c`, `drivers/regulator/qti-ocp-notifier.c`, `drivers/thermal/qcom/bcl_pmic5.c` |
| MiCode Uke DT | `94c84aeb9e517dbde546fc732033c184094c8e57`; `qcom/uke-sm8635.dtsi`, `cliffs.dtsi`, `cliffs-pm7550ba.dtsi`, `cliffs-pmic-overlay.dtsi` |
| Nord4 shared-SoC | `fc892483f1d61f245924681eb5abd563564d9cba`; QTI transport, distinct firmware/board contract |
| Peridot shared-IP | `2ed9eadbecae21ef51134e556951e7305ba65f88`; mainline battmgr, distinct board/topology |
| Retained upstream | `4056b3c9d3c92ea45955641dbafeb9b960e146ef`; standard battmgr and direct-I2C charger boundaries |

QTI files retain GPL-2.0-only and Qualcomm attribution; the generic BQ25980
driver has GPL-2.0 and Texas Instruments attribution. No donor implementation
or board limit was copied. The reviewed Uke board fragment has no file-level
license header; no DT content was imported. Current upstream raw battmgr and
BQ25980 pages were inspected separately; exact-revision downloads were rate-
limited, so the current commit lookup is not a new validated source lock.

## Source inventory and remaining work

`OSS-CHARGING-INVENTORY.tsv` lists all 354 named entries in the reviewed battery
(30), USB (14), wireless (12), Xiaomi (262) and DFX (36) enums, including
sentinels/reserved markers and conditional anti-burn. This is a lexical inventory,
not 354 supported firmware properties. Numeric IDs are omitted: active macros,
firmware variant and the native Uke battery numbering require separate proof.

| Area | Coverage and remaining requirement |
|---|---|
| Fitted pump/charger and topology | Establish actual part/revision, bus owner, phase count and pack/gauge basis before selecting a driver. |
| Pump bus/battery OVP/OCP | OEM names exist; native fault/limit/effect path unqualified. Establish autonomous configuration/action. |
| Pump UVLO/UCP, reverse current and short | Exact fitted-part applicability and response unknown; no borrowed register bits or limits. |
| Pump die shutdown and NTC faults | No calibrated sensor mapping or open/short/shutdown acceptance. |
| Pump/main-charger watchdogs | Timing/reset/expiry contract unknown; AP lease renewal is not a hardware watchdog. |
| Dual-pump coordination | Master/slave names do not prove fitted topology, sharing or safe phase loss. |
| CP fault notification | OEM AP handler empty; establish actual message ABI/delivery/lifecycle and independent firmware action. |
| CV, JEITA, precharge, termination/recharge | Firmware-owned; exact pack envelope and autonomous behavior unqualified. |
| Charge safety timer | Selected timeout, reset causes and autonomous failure behavior unknown. |
| Pack protector, NTC, short and imbalance | Standard Health/temperature are observations; establish fitted protector/topology and independent action. |
| Pack/gauge/adapter authentication | Digest/authentic names are not implementation; genuine MI FC2 authentication remains missing. |
| Connector anti-burn, CC short and corrosion | Existing moisture/IIO/inhibit paths remain guarded; exact anti-burn macro/ABI and response unqualified. |
| PM7550BA BCL | Native hwmon/IRQ source and prior bounded observations exist; calibration, alarms/wake and cooling effectiveness open. |
| PMXR2230 BCL | Separate block; PM7550BA scaling/register behavior cannot be assumed. |
| Regulator OCP/undervoltage notification | OEM notifier reads PMIC NVMEM logs and notifies consumers; native equivalent not established. Notification is not cutoff. |
| Age/cycle/thermal derating | DT tables are exported by `ut_test_show()`; consuming policy is not established. Export alone is not firmware programming. |
| Low-voltage shutdown/reserve | OEM voltage/delay workaround differs from percentage shutdown; reserve and firmware cutoff unqualified. |
| Firmware restart/suspend/AP death | PDR/epoch guards exist; autonomous protection and safe defaults still need evidence. |
| OEM night/smart/sport/fast policies | Unqualified semantics; no undocumented flags substituted for daily thresholds. |
| Decimal SOC and FG/FG2 health/learning | Unknown cell mapping/units/calibration; no fabricated health or automatic learning. |
| Wireless/pen/reverse | Shared source does not establish retail hardware/accessory applicability. |
| Charger partition | OEM helper can issue persistent SCSI writes. Storage mutation is not a protection to enable blindly; no access/port added. |
| Factory/debug/fake/MTBF/ship/learning controls | Deliberately excluded; no fault clear, protection disable or factory override added to fill an inventory row. |

## Admission and physical boundary

The existing profile now requires `proofs.integrated_protection_inventory`, a
SHA-256 reference to reviewed evidence. An otherwise valid legacy profile
without it is refused, as are null/malformed references. Changed profile bytes
invalidate the prior per-boot attestation. There is no automatic migration or
operator bypass. Shipped profiles and service presets remain unchanged/closed.

The review must cover every applicable `protection-readiness.json` feature:
fitted parts/firmware/pack and owners, enabled protection configuration, units
and limits, autonomous fault action, latch/reset conditions, boot/source/suspend/
restart/AP-death behavior and uncertainties. Inapplicability requires board-
specific evidence; unknown applicability is not an exemption. This digest is a
root-reviewed evidence reference, not hardware authentication or automatic
verification of its conclusions.

A generic pump probe can program watchdog and OVP/current registers and conflict
with remote firmware before a charging request. Conversely, a protected board
may have no AP pump driver because firmware owns it. Establish the owner and
enabled autonomous behavior first, then port an evidenced missing path. A fault
name, zero alarm, successful reply or Health=Good cannot establish protection.

## Manufacturer document review, 29 October 2026

`CHARGING-DOCUMENTS.tsv` records manufacturer URLs, revisions, availability and
SHA-256 identities of privately retained PDF bytes. Every part remains an
unconfirmed Uke candidate. The catalogue is research input; it supplies no
pack limits, register addresses, firmware property IDs or qualification.
The detailed comparison is recorded in the canonical
[manufacturer review](https://github.com/MCC45TR/uke-linux-docs/blob/main/docs/lessons/2026-10-29-UKE-CHARGING-DATASHEETS.md).

The existing integrated-protection review must also establish read side effects
and exclusive fault ownership. BQ25960 SLUSE08A sections 8.3.7 and 8.3.11 show
why: register traffic can maintain its watchdog, while reading FLAG consumes
latched events. A diagnostic must not become another watchdog feeder or clear
the owner's evidence. Use reviewed cached observations from the actual owner;
do not run bus discovery, register sweeps or a second pump driver to identify
an unknown part. This finding does not establish the effects of Uke GLINK GETs.

Document alarm versus autonomous shutdown, interrupt masking versus protection
disable, reset/expiry defaults and recovery separately. A vendor's bypass path
can still charge the battery. It does not qualify KDE's charge-hold state or
whole-battery isolation. Public abbreviated documents and another part's
register map cannot qualify a missing protection. Keep gauge firmware variants,
calibration, vendor SOH and same-basis capacity retention distinct.

Next device work needs a matched signed delivery, rollback and an attended
window. Identify/read configuration using a proven read-only contract first.
No short, overvoltage, overheating, wet-connector, watchdog-reset or forced
deep-discharge experiment is part of this software validation.
