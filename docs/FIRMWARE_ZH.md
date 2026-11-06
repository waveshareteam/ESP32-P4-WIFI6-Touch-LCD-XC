# 固件源码边界

[English](FIRMWARE.md)

[`firmware/brookesia`](../firmware/brookesia/) 是本开发板系列维护中的
ESP-Brookesia 交付源码工程。它以 LCD-X 固件源码布局为基线并适配 XC 开发板，
不属于 `examples/` 下的普通示例。

## 配置与构建

固件默认面向 ESP32-P4 rev3.x 和 32 MB Flash。对于 pre-v3 芯片，在基础配置之后加载 `sdkconfig.defaults.rev1_3`，选择最低版本 1.0 和 200 MHz PSRAM。修改默认配置后请检查生成的 `sdkconfig`。

原有的 `sdkconfig.defaults.3_4c` 和 `sdkconfig.defaults.4c` 已删除。当前本地 Board Manager 生成配置使用 720 × 720 屏幕；800 × 800 屏幕使用 Board Manager amend `lcd_3_4_dsi_touch_c`。编译和烧录说明见[固件 README](../firmware/brookesia/README_ZH.md)。

固件使用 `waveshare/waveshare_boards`、`espressif/esp_board_manager` 和本地 `board` 组件。发布镜像前应记录解析后的依赖版本，并验证所选板级配置。

## 出厂镜像

当前工作区包含 `ESP32-P4-WIFI6-Touch-LCD-3.4C-FactoryOnly-261106.bin` 和 `ESP32-P4-WIFI6-Touch-LCD-4C-FactoryOnly-261106.bin`；本次改动删除了旧的 `260821` 文件。发布前请确认各镜像的构建来源及对应的实机验证结果。

## CI 与验证

维护固件工作流与示例矩阵分离。CI 为两个屏幕生成板级配置，并构建两个芯片版本配置。构建成功本身不代表硬件功能已验证。
