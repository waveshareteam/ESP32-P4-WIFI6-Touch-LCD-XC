# Hardware Reference and Audit

[中文](HARDWARE_ZH.md)

This repository includes the board schematic at
[`hardware/schematics/ESP32-P4-WIFI6-Touch-LCD-XC-Schematic.pdf`](../hardware/schematics/ESP32-P4-WIFI6-Touch-LCD-XC-Schematic.pdf).
It is the primary local reference for changes involving pins, connectors, power,
display, touch, camera, audio, storage, USB, or the ESP32-C6 wireless module.

## Evidence in this repository

The two-page schematic contains the ESP32-P4 and ESP32-C6 sections, Type-C and
USB-UART/USB-OTG interfaces, microSD, CSI, the 3.4/4-inch display connector,
codec/ADC, microphone, speaker amplifier, reset/boot controls, and power rails.
The following maintained sources provide the software-side contract:

| Surface | Repository evidence |
| --- | --- |
| ESP-IDF board support | Dependency manifests such as `examples/esp-idf/07_Displaycolorbar/main/idf_component.yml`, `examples/esp-idf/08_lvgl_demo_v9/components/bsp_extra/idf_component.yml` pin the managed `waveshare/esp32_p4_wifi6_touch_lcd_xc` BSP to `3.0.1`; maintained firmware instead uses `waveshare/waveshare_boards` and Board Manager |
| Display variants | BSP headers plus `BSP_LCD_TYPE_800_800_3_4_INCH` / `BSP_LCD_TYPE_720_720_4_INCH` configuration |
| Arduino display variants | `examples/arduino/libraries/displays/displays_config.h` and `CURRENT_SCREEN` in the first-party sketches |
| Arduino I2C/touch | `examples/arduino/libraries/displays/i2c.h` and `gt911.h` |
| Camera example | `examples/esp-idf/09_video_lcd_display/sdkconfig.defaults` and the local `esp_video` manifest |
| Hosted Wi-Fi | `examples/esp-idf/04_wifistation/main/idf_component.yml` |

The examples use GT911-compatible touch APIs. The repository intentionally does
not duplicate a complete pin table in this document: the schematic, managed BSP
source, and Arduino configuration are the sources to update together when a
board-facing change is made.

## Current static audit

| Interface | Static contract and boundary |
| --- | --- |
| Display | 3.4C uses `BSP_LCD_TYPE_800_800_3_4_INCH`, 4C uses `BSP_LCD_TYPE_720_720_4_INCH`; both use two MIPI-DSI lanes at 1,500 Mbps/lane with an 80 MHz DPI clock. LCD reset is GPIO27 and backlight PWM is GPIO26. |
| I2C | SDA is GPIO7 and SCL is GPIO8. |
| Touch | The official controller is GT9271; the software uses a GT911-compatible driver/API. `TP_RST`/`CTP_RESET` reaches GPIO23 through 0-ohm R62, while `TP_INT`/`CTP_INT` reaches only TP2 with no MCU route. Software deliberately leaves both pins `GPIO_NUM_NC`, installs no ISR, probes `0x5D` then `0x14`, and polls with `esp_lcd_touch_read_data()`. Leaving reset unconfigured avoids changing the address/reset strap behavior. |
| microSD | SD D0..D3 use GPIO39..GPIO42, CLK GPIO43, and CMD GPIO44; this matches the BSP contract. |
| Audio | ES8311/ES7210 use I2S GPIO9..GPIO13 and PA enable GPIO53; this matches the BSP contract. |
| Memory | ESP32-P4NRW32 has 32 MB in-package PSRAM and the GD25Q256 provides 32 MB flash; this matches the configured memory profile. |
| Processor, wireless, and revisions | The schematic identifies the ESP32-P4 and ESP32-C6 board design. Its board revision is rev1.1. `rev1_3` and `rev3_x` are ESP32-P4 silicon compatibility profiles, not PCB revisions. Pre-v3 selects the legacy PLL_F20M DSI PHY reference; rev3.x selects XTAL. `.phy_clk_src = 0` lets ESP-IDF choose this source while preserving the 80 MHz DPI clock. |

## Audit rules for future changes

Before changing a hardware constant or board-facing README:

1. Identify the affected board interface in the schematic.
2. Compare the schematic net names with the BSP header, Arduino configuration,
   `sdkconfig.defaults`, and example source.
3. Check both display resolutions and both Arduino `CURRENT_SCREEN` variants when
   the change affects the display path.
4. Keep the PHY reference source profile-dependent: never force the legacy
   PLL_F20M source for a rev3.x build. Arduino and the managed BSP use the
   automatic selection path.
5. Record whether validation is static (source/schematic) or includes a physical
   board test. A successful CI build proves compilation, not pin correctness.

The managed BSP remains at published `3.0.1`, which already provides the
dual-address, no-pin, polling touch contract; no unpublished `3.0.2` is needed.
This static audit and a successful compile do not prove a board transaction.
HIL remains required for the responding address, coordinates, release events,
and polling behavior on both display variants.
