# Charging protocol coverage

29 October 2026. Source and isolated host validation for POCO Pad X1, Fedora
Rawhide AArch64 target. No tablet access, firmware request or physical charging
test was performed. Firmware retains negotiation and electrical protection.

The [manufacturer receiver FAQ](https://www.mi.com/ph/support/faq/details/KA-628084/)
lists QC2.0, QC3.0, QC3+, PD2.0, PD3.0 and MI FC2.0, and explicitly excludes
QC4/4+, PPS and UFCS. The charger protocol list is separate. Neither advertised
45 W nor generic PD establishes a session revision, actual input power or PPS.

| Protocol | Linux source preparation | Remaining acceptance |
|---|---|---|
| QC2.0 | Explicit USB7/USB11 OEM class inspection; firmware negotiation retained | Qualified QC2 adapter/cable, active ABI, session and measured envelope |
| QC3.0 | Same path preserves HVDCP_3 without relabelling ordinary DCP | Exact session, transition, limits and fallback |
| QC3+ | HVDCP_3P5 and CLASSB remain distinct reported OEM names | Exact QC3+ mapping/session; no inference from class name alone |
| PD2.0 | Coherent cached UCSI PDO/RDO and bounded fixed-supply decoding retained | Actual revision/PS_RDY and source/cable/electrical measurements |
| PD3.0 | Same UCSI owner; no second negotiation engine | Actual negotiated revision; PD3 capability does not establish PPS |
| MI FC2.0 | Reported as manufacturer-listed, with missing genuine native authentication backend | Reviewed native authentication/session/restore implementation and OEM adapter/cable qualification |
| QC4/4+, PPS, UFCS | Explicit manufacturer exclusion in the report | Not enabled by generic firmware strings or charger advertising |

`uke-charge-control protocols` remains cache-only. `protocol-inspect` is a
separate explicit root command, also routed by `uke-control charge
protocol-inspect`. It requires the default-off EXPERT option
`CONFIG_BATTERY_QCOM_BATTMGR_UKE_PROTOCOL_DIAGNOSTICS`. Normal configuration does
not enable it. Do not enable or invoke it until the active firmware ABI and an
attended test window are reviewed.

The interface reads only USB family GET0x32 properties7 and11. It refuses a
busy lock, unstable transport, active charging control, rapid repetition and
changed epochs. Each successful result is checked against provider, boot,
kernel, driver/firmware/source identity and time before reporting. The pair is
sequential, not a remotely atomic sample. No normal status poll triggers it.
Unknown values remain raw and labelled Unrecognized. OEM enum9 is Apple Brick
ID, while vendor128-132 retain HVDCP/HVDCP_3/HVDCP_3P5/USB_FLOAT/CLASSB names.
USB6, USB7, USB11, XM real_type and current mainline enum9+ are different ABIs.
No class is promoted into verified negotiation, authentication or safe limits.

OEM OSS `qti_battery_charger.c` at
`5712080a4b0c0b1a879e6d145c601c649ddaddfd` supplies these fixed USB IDs and
separate XM/UVDM interfaces. The OS3.0.304.0 stock module independently retains
USB11 and the vendor-name path. Read-only ADSP inspection finds QC/PD/UVDM
machinery, along with generic PPS/UFCS strings; no selected-profile or active
firmware identity is inferred. Existing GPL source attribution is preserved.

The stock `batterysecret` and MiCharge executables use Android's linker and
Binder/HIDL/Bionic dependencies. They are research evidence, not Fedora
executables or a native authentication implementation. No proprietary binary,
authentication secret, forced verified flag or donor script is installed or run.
The firmware lock and default unqualified charging profiles remain unchanged.

Host fixtures cover the actual fixed-GET helper, default-off behavior, root
permissions, rate/lease/epoch faults, strict native parsing, all reported OEM
classes, unknown values and CLI routing. Kernel ARM64 translation and native
client checks are separate from binary delivery and physical protocol acceptance.
