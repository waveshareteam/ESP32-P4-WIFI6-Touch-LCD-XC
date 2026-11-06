# Component Ownership and Dependency Notes

[中文](COMPONENTS_ZH.md)

This page records why local component candidates are kept in their current
boundaries. A directory name alone is not treated as evidence that a component
can be removed.

## Registry-backed board support

Examples 07 through 12 use the managed
[`waveshare/esp32_p4_wifi6_touch_lcd_xc`](https://components.espressif.com/components/waveshare/esp32_p4_wifi6_touch_lcd_xc)
BSP pinned exactly to the published `3.0.1` release. No vendored copy of that
BSP remains in an example; `bsp_extra` and unrelated local components remain
local. Component manifests must use a published registry version: do not pin an
unpublished version or a Git/local-path dependency, because either form is
rejected by the Component Registry packaging flow.

The schematic gives an exact static contract for the touch signals: the display
connector's `TP_RST`/`CTP_RESET` is tied through 0-ohm `R62` to GPIO23, while
`TP_INT`/`CTP_INT` goes only to the `TP2` test point and has no MCU route. The
published `3.0.1` BSP deliberately sets both touch reset and interrupt to
`GPIO_NUM_NC`, installs no interrupt handler, probes I2C address `0x5D` and
then `0x14`, and initializes the address that responds. This prevents software
from altering the GT9271 address/reset strap behavior while retaining a polling
touch path. No `3.0.2` dependency is required: `3.0.1` already implements this
contract, so this migration does not invent a local GPIO override or re-vendor
the component.

Compile and static checks establish only the declared software contract. Real
3.4C and 4C hardware is still required to verify the responding touch address,
coordinates, release events, and polling behavior before declaring touch HIL
validation complete.

The USB extend-screen example also pins `espressif/tinyusb` to `0.17.0~2`, the
exact release permitted by its `espressif/usb_device_uac` `1.2.0` dependency.
Keeping both versions exact prevents a future TinyUSB upload from silently
changing USB descriptors or P4 PHY behavior. The UAC component dependency is
conditional on the top-level `USB_DEVICE_UAC_COMPONENT` CMake option. Normal
builds leave it enabled; the CI vendor-only command disables both that option
and `CONFIG_UAC_AUDIO_ENABLE`, so it does not compile a component whose
descriptor types are disabled in the project TinyUSB configuration. The CMake
option is used because Kconfig-based manifest conditions require ESP-IDF 6.0
and this repository also validates ESP-IDF 5.5; see the Component Manager's
[Kconfig condition documentation](https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/manifest_file.html#kconfig-options).

## Compatibility ranges and revisit conditions

- Example 04 deliberately uses separate hosted-Wi-Fi ranges. ESP-IDF 6 uses
  `esp_wifi_remote >=1.6,<2.0` with `esp_hosted >=2.12,<3.0`; ESP-IDF 5.5 uses
  `esp_wifi_remote 0.14.*` with `esp_hosted 1.4.*`. Revisit these ranges only
  when the exact ESP32-C6 image or source revision is recorded and both ESP-IDF
  lines pass build and hardware-in-the-loop checks.
- Example 09 keeps `esp_video ~2.0`. Move that range only after the camera and
  display pipeline builds on both ESP-IDF lines and passes hardware validation.
- Example 10 pins the published `esp_audio_codec 2.5.0`. Versions 2.6 and later
  require ESP32-P4 revision >= 3.0, so using them would break the explicit
  `rev1_3` compatibility profile even though `rev3_x` is the example default.
- Example 08 accepts LVGL v9 with `^9.*`; example 12 and the Brookesia surfaces
  use LVGL `9.5.0`. Move these contracts only after compile, UI, display and
  touch regression checks on both display variants.
- The maintained firmware uses `waveshare/waveshare_boards` and `esp_board_manager`. Record resolved versions and validate the selected board configuration before publishing firmware.

## Product-local or example-local components

| Component | Current reason to keep it local |
| --- | --- |
| `examples/esp-idf/05_sdmmc/components/sd_card` | Example-specific SD test helper and GPIO test routines |
| `examples/esp-idf/08_lvgl_demo_v9/components/bsp_extra` | Board/demo glue around audio and display integration |
| `examples/esp-idf/10_mp4_player/components/esp_extractor` | Espressif extractor integration with target-specific prebuilt libraries used by this example |
| `examples/esp-idf/11_esp_brookesia_phone/components/brookesia_app_squareline_demo` | Example application composition |
| `examples/esp-idf/12_usb_extend_screen/components/bsp_extra` | USB/display example-specific board glue |

The `firmware/brookesia/components/` tree is a separate maintained firmware surface. Its local `board` component wraps Board Manager devices; firmware CI is separate from the example matrix.

When a reusable correction is needed in the shared Waveshare component
repository, request authorization for that upstream change before publishing a
new dependency release. Do not silently replace local board glue with a
component that has not been checked against the schematic and both display
variants.
