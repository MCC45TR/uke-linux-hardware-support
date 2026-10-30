# Light interfaces and desktop compatibility

30 October 2026. Source candidate Control 2.4.0-2 and Plasma controls release 26.
No runtime installation or new physical acceptance is claimed here.

## Integer torch requests and the Nabu comparison

The request range is every integer 1–100, with zero reserved for off. The
kernel remembers the exact successful request. The widget permits direct
integer entry; the CLI accepts, for example, `flashlight set 47`, `51` or `52`.
Status separates `requested_percent` from applied `brightness_percent` and
advertises request step 1, provider scale 2 and upward rounding.

The [Nabu helper](https://github.com/MCC45TR/nabu-tablet-controls/blob/08ac8edd501a36d89aef1e5dadbafe5f2a6de79f/src/nabu-flashlight.c)
uses `(max_brightness * percent + 99) / 100`. Its
[GNOME slider](https://github.com/MCC45TR/nabu-tablet-controls/blob/08ac8edd501a36d89aef1e5dadbafe5f2a6de79f/gnome/extension.js)
rounds input to integers and coalesces requests. Nabu's
[kernel provider](https://github.com/MCC45TR/nabu-linux-kernel/blob/0b3226fc8f3cab5efb8a47bd17f5d7bd3ed609d1/drivers/leds/flash/leds-qcom-flash.c)
uses the 255 software scale, then converts to current with 5 mA granularity.
Its [DT](https://github.com/MCC45TR/nabu-linux-kernel/blob/0b3226fc8f3cab5efb8a47bd17f5d7bd3ed609d1/arch/arm64/boot/dts/qcom/sm8150-xiaomi-nabu.dts)
admits a 200 mA single-channel torch. A 1–100 slider therefore does not establish
100 distinct physical current levels.

Uke now uses the same upward percentage conversion inside its kernel owner.
Its reviewed two-channel 20 mA total limit is retained:

| Request | Retained request | Applied provider level | Total programmed current |
|---|---|---|---|
| 47 | 47 | 1 of 2 | 10 mA |
| 50 | 50 | 1 of 2 | 10 mA |
| 51 | 51 | 2 of 2 | 20 mA |
| 52 | 52 | 2 of 2 | 20 mA |
| 100 | 100 | 2 of 2 | 20 mA |

These are driver-programmed values, not measured light/current results. Nabu's
current limit, root helper and direct sysfs writes are not imported. No software
PWM or unsupported current resolution is introduced. A genuinely finer optical
range needs a separately evidenced hardware mode and electrical qualification.

## RGB channel and brightness API

The existing kernel ABI `/dev/uke-rgb`, `UKE_RGB_SET`, accepts
`struct uke_rgb_color { abi, rgb, brightness_percent, reserved; }` with fixed
32-bit fields. `rgb` is `0xRRGGBB`, each channel 0–255; brightness is 0–100.
No private kernel ABI change or new ioctl permission is needed.

The authorized public system-bus endpoint is `org.senemos.Uke.Rgb`, object
`/org/senemos/Uke/Rgb`, interface `org.senemos.Uke.Rgb1`:

| Method | Input | Result |
|---|---|---|
| `Status` | none | JSON string; read-only |
| `SetRgb` | `uuuu`: red, green, blue, brightness percent | JSON string after the admitted kernel request |
| `SetColor` | `su`: compatible HEX, brightness percent | Same kernel request |
| `Automatic` | none | Clear manual/test state; keep automatic warnings |
| `TestColors` | none | One kernel-owned ten-second color test |

`uke-control led set 255 255 255 52` uses the typed decimal method. The widget's
R/G/B and brightness integer controls use the same persistent connection.
Status adds `color_rgb`, `requested_color_rgb`, `requested_percent`,
`native_brightness`, `native_brightness_limit` and `policy_owner`. Applied
state can lag the accepted request and remains distinct from optical acceptance.
Brightness 100 means the unchanged 8/511 native PWM ceiling, not unrestricted
511/511. Zero clears manual control; camera and battery policy retain priority.

Every mutation goes through the existing asynchronous Polkit authorization.
Another bus peer cannot reuse a connection-bound grant. No UID-wide grant,
direct device access or generic sysfs writer is added.

## Original desktop consumers

The existing `uke:white:torch` and `uke:rgb:status` providers already expose
standard LED/multicolor metadata. Discovery and write authorization are separate.

| Original consumer | Reviewed implementation | Candidate limitation |
|---|---|---|
| Uke applet in original Plasma | Fixed persistent Qt broker client | Updated integer torch and decimal RGB controls; session/device acceptance pending |
| Plasma Mobile flashlight | [LED udev discovery and KAuth helper](https://github.com/KDE/plasma-mobile/blob/master/quicksettings/flashlight/flashlightutil.cpp) | Finds `*:torch`/`*:flash`; its helper writes LED sysfs, not this broker |
| Phosh torch | [LED discovery and logind SetBrightness](https://gitlab.gnome.org/World/Phosh/phosh/-/blob/main/src/torch-manager.c) | Its logind write does not carry the original application peer into this broker |

The managed consumers deliberately reject competing sysfs stores. Making these
native mobile buttons work by granting world-write permission or trusting every
active-session client would bypass the requested authorization boundary. Their
native backends need broker-aware integration before they can control this
candidate. This is an open compatibility gate, not working mobile-shell support.
No KDE application is cloned, forked or rebuilt, and no system helper or bus name
is impersonated.
