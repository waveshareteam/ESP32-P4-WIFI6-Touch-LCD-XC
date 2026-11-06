# 组件归属与依赖说明

[English](COMPONENTS.md)

本文记录本地组件候选为何保留在当前边界内。不能仅凭目录名称判断一个组件
是否可以删除。

## 注册表板级支持

示例 07 至 12 使用托管的
[`waveshare/esp32_p4_wifi6_touch_lcd_xc`](https://components.espressif.com/components/waveshare/esp32_p4_wifi6_touch_lcd_xc)
BSP，并精确固定为已发布的 `3.0.1` 版本。示例中不再保留该 BSP 的 vendored 副本；
`bsp_extra` 和其他无关本地组件仍保持本地边界。组件 manifest 必须使用已发布的注册表
版本：不要固定未发布版本，也不要使用 Git/本地路径依赖，因为这两类依赖都会被
Component Registry 打包流程拒绝。

原理图对触控信号给出了确切的静态约定：显示连接器的 `TP_RST`/`CTP_RESET` 通过
0 欧 `R62` 连接到 GPIO23；`TP_INT`/`CTP_INT` 只连接到 `TP2` 测试点，没有 MCU
路由。已发布的 `3.0.1` BSP 有意将触控复位和中断都设为 `GPIO_NUM_NC`，不安装中断
处理器，依次探测 I2C 地址 `0x5D` 和 `0x14`，并用有响应的地址初始化。这样软件不会
改变 GT9271 的地址/复位 strap 行为，同时保留轮询触控路径。无需 `3.0.2` 依赖：
`3.0.1` 已实现该约定，因此本次迁移不会凭空添加本地 GPIO 覆盖，也不会重新 vendoring
该组件。

编译和静态检查只能确认已声明的软件约定。要完成触控 HIL 验证，仍需在真实 3.4C 和
4C 开发板上确认有响应的触控地址、坐标、抬起事件和轮询行为。

USB 扩展屏示例还把 `espressif/tinyusb` 精确固定为 `0.17.0~2`，这是现有
`espressif/usb_device_uac` `1.2.0` 允许的精确版本。两者都使用精确版本，可避免
未来 TinyUSB 上传静默改变 USB 描述符或 P4 PHY 行为。UAC 组件依赖受
顶层 `USB_DEVICE_UAC_COMPONENT` CMake 选项控制。常规构建保持启用；CI 的
vendor-only 命令同时关闭该选项与 `CONFIG_UAC_AUDIO_ENABLE`，因此不会再编译一个
其描述符类型已被项目 TinyUSB 配置关闭的组件。这里使用 CMake 选项，是因为基于
Kconfig 的 manifest 条件要求 ESP-IDF 6.0，而本仓库还要验证 ESP-IDF 5.5；详情见
Component Manager 的
[Kconfig 条件说明](https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/manifest_file.html#kconfig-options)。

## 兼容版本范围与重访条件

- 示例 04 有意为 Hosted Wi-Fi 使用两组版本范围。ESP-IDF 6 使用
  `esp_wifi_remote >=1.6,<2.0` 与 `esp_hosted >=2.12,<3.0`；ESP-IDF 5.5
  使用 `esp_wifi_remote 0.14.*` 与 `esp_hosted 1.4.*`。只有在记录准确的
  ESP32-C6 镜像或源码 revision，并且两个 ESP-IDF 版本线都通过构建和 HIL
  验证后，才重访这些范围。
- 示例 09 保持 `esp_video ~2.0`。只有摄像头和显示链路在两个 ESP-IDF 版本线
  均构建通过并完成硬件验证后，才调整该范围。
- 示例 10 精确固定已发布的 `esp_audio_codec 2.5.0`。2.6 及更高版本要求
  ESP32-P4 revision >= 3.0；即使示例默认使用 `rev3_x`，升级也会破坏显式的
  `rev1_3` 兼容 profile。
- 示例 08 以 `^9.*` 接受 LVGL v9；示例 12 与 Brookesia 面使用 LVGL
  `9.5.0`。只有两个显示变体都通过编译、UI、显示与触控回归后，才调整这些约定。
- 维护固件使用 `waveshare/waveshare_boards` 和 `esp_board_manager`。发布固件前，应记录解析后的组件版本并验证所选板级配置。

## 产品或示例本地组件

| 组件 | 当前保留本地的原因 |
| --- | --- |
| `examples/esp-idf/05_sdmmc/components/sd_card` | 示例专用的 SD 测试辅助和 GPIO 测试逻辑 |
| `examples/esp-idf/08_lvgl_demo_v9/components/bsp_extra` | 音频/显示集成所需的板级与示例 glue |
| `examples/esp-idf/10_mp4_player/components/esp_extractor` | 本示例使用的 Espressif extractor 集成及按目标提供的预编译库 |
| `examples/esp-idf/11_esp_brookesia_phone/components/brookesia_app_squareline_demo` | 示例应用组合逻辑 |
| `examples/esp-idf/12_usb_extend_screen/components/bsp_extra` | USB/显示示例专用的板级 glue |

`firmware/brookesia/components/` 是单独维护的固件源码面。本地 `board` 组件封装 Board Manager 设备；固件 CI 与示例矩阵分离。

如果可复用的修复应当进入 Waveshare 共享组件仓库，应先获得上游修改和发布新依赖
版本的授权。未经原理图和两个显示变体核对，不要静默用共享组件替换本地板级 glue。
