# Firmware

[English](README.md) | [简体中文](README_ZH.md)

Select the directory that matches your board model to download the factory firmware and view flashing instructions. Each model has its own image; use the firmware for your board's screen size.

| Model | Firmware and flashing instructions |
| --- | --- |
| ESP32-P4-WIFI6-Touch-LCD-3.4C | [Download and flash](ESP32-P4-WIFI6-Touch-LCD-3.4C/README.md) |
| ESP32-P4-WIFI6-Touch-LCD-4C | [Download and flash](ESP32-P4-WIFI6-Touch-LCD-4C/README.md) |

## Firmware manifest

[`manifest.json`](manifest.json) lists the BIN and bilingual instructions for each model. Paths are relative to the repository root. When adding a firmware version, list its BIN path under the corresponding model and update its two README files. Keep the paths of versions that should remain available in the firmware center.
