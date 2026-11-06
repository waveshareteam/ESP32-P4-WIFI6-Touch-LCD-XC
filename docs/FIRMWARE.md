# Firmware Source Boundary

[中文](FIRMWARE_ZH.md)

[`firmware/brookesia`](../firmware/brookesia/) is the maintained ESP-Brookesia
delivery-source project for this board family. It is based on the LCD-X firmware
source layout and adapted for the XC board; it is not a routine example under
`examples/`.

## Configuration and builds

The firmware defaults target ESP32-P4 rev3.x and 32 MB flash. For pre-v3 silicon, load `sdkconfig.defaults.rev1_3` after the base defaults to select revision 1.0 and 200 MHz PSRAM. Check the generated `sdkconfig` after changing defaults.

The former `sdkconfig.defaults.3_4c` and `sdkconfig.defaults.4c` files have been removed. The current local Board Manager generated configuration uses a 720 × 720 display; select the 800 × 800 panel with Board Manager amendment `lcd_3_4_dsi_touch_c`. See the [firmware README](../firmware/brookesia/README.md) for build and flash instructions.

The firmware uses `waveshare/waveshare_boards` with `espressif/esp_board_manager` and the local `board` component. Record the resolved dependency versions and validate the selected board configuration before distributing an image.

## Factory images

The working tree contains `ESP32-P4-WIFI6-Touch-LCD-3.4C-FactoryOnly-261106.bin` and `ESP32-P4-WIFI6-Touch-LCD-4C-FactoryOnly-261106.bin`. The previous `260821` files are removed in the current changes. Confirm each image's build provenance and matching hardware validation before release.

## CI and validation

The maintained firmware workflow is separate from the example matrix. CI generates the board configuration for both panel variants and builds both silicon profiles. A successful build alone does not establish hardware behavior.
