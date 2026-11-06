# 硬件参考与审计

[English](HARDWARE.md)

本仓库包含开发板原理图：
[`hardware/schematics/ESP32-P4-WIFI6-Touch-LCD-XC-Schematic.pdf`](../hardware/schematics/ESP32-P4-WIFI6-Touch-LCD-XC-Schematic.pdf)。
涉及引脚、连接器、电源、显示、触控、摄像头、音频、存储、USB 或 ESP32-C6
无线模块的修改，都应以它作为本地主要参考。

## 仓库内证据

两页原理图包含 ESP32-P4、ESP32-C6、Type-C、USB-UART、USB-OTG、microSD、
CSI、3.4/4 英寸显示屏连接器、Codec/ADC、麦克风、扬声器功放、复位/启动控制
和电源部分。以下维护中的源码提供软件侧约定：

| 范围 | 仓库证据 |
| --- | --- |
| ESP-IDF 板级支持 | `examples/esp-idf/07_Displaycolorbar/main/idf_component.yml`、`examples/esp-idf/08_lvgl_demo_v9/components/bsp_extra/idf_component.yml` 等依赖 manifest 将托管的 `waveshare/esp32_p4_wifi6_touch_lcd_xc` BSP 固定为 `3.0.1`；维护固件则使用 `waveshare/waveshare_boards` 和 Board Manager |
| 显示屏变体 | BSP 头文件和 `BSP_LCD_TYPE_800_800_3_4_INCH` / `BSP_LCD_TYPE_720_720_4_INCH` 配置 |
| Arduino 显示变体 | `examples/arduino/libraries/displays/displays_config.h` 和一方示例中的 `CURRENT_SCREEN` |
| Arduino I2C/触控 | `examples/arduino/libraries/displays/i2c.h` 和 `gt911.h` |
| 摄像头示例 | `examples/esp-idf/09_video_lcd_display/sdkconfig.defaults` 及本地 `esp_video` manifest |
| Hosted Wi-Fi | `examples/esp-idf/04_wifistation/main/idf_component.yml` |

示例使用 GT911 兼容触控 API。本文档不重复维护完整引脚表；如果修改开发板
相关内容，应同时更新和核对原理图、托管 BSP 源码及 Arduino 配置。

## 当前静态审计

| 接口 | 静态约定与边界 |
| --- | --- |
| 显示 | 3.4C 使用 `BSP_LCD_TYPE_800_800_3_4_INCH`，4C 使用 `BSP_LCD_TYPE_720_720_4_INCH`；两者均使用两条 1,500 Mbps/lane 的 MIPI-DSI 通道和 80 MHz DPI 时钟。LCD 复位为 GPIO27，背光 PWM 为 GPIO26。 |
| I2C | SDA 为 GPIO7，SCL 为 GPIO8。 |
| 触控 | 官方控制器为 GT9271；软件使用 GT911 兼容驱动/API。`TP_RST`/`CTP_RESET` 经 0 欧 R62 连接到 GPIO23；`TP_INT`/`CTP_INT` 只连接到 TP2，没有 MCU 路由。软件有意将两个引脚都保留为 `GPIO_NUM_NC`，不安装 ISR，依次探测 `0x5D` 和 `0x14`，并通过 `esp_lcd_touch_read_data()` 轮询。复位不由软件配置，以避免改变地址/复位 strap 行为。 |
| microSD | SD D0..D3 使用 GPIO39..GPIO42，CLK 为 GPIO43，CMD 为 GPIO44；与 BSP 约定一致。 |
| 音频 | ES8311/ES7210 使用 I2S GPIO9..GPIO13，PA 使能为 GPIO53；与 BSP 约定一致。 |
| 存储器 | ESP32-P4NRW32 具有 32 MB 封装内 PSRAM，GD25Q256 提供 32 MB Flash；与配置的存储 profile 一致。 |
| 处理器、无线和版本 | 原理图标识了 ESP32-P4 和 ESP32-C6 的开发板设计，其开发板版本为 rev1.1。`rev1_3` 和 `rev3_x` 是 ESP32-P4 芯片兼容 profile，不是 PCB 版本。pre-v3 选择旧版 PLL_F20M DSI PHY 参考源，rev3.x 选择 XTAL；`.phy_clk_src = 0` 让 ESP-IDF 选择该来源，同时保持 80 MHz DPI 时钟。 |

## 后续修改的审计规则

修改硬件常量或面向开发板的 README 前：

1. 在原理图中定位受影响的板级接口。
2. 将原理图网络名与 BSP 头文件、Arduino 配置、`sdkconfig.defaults` 和示例源码
   对照。
3. 影响显示路径时，同时检查两个显示分辨率和两个 Arduino `CURRENT_SCREEN` 变体。
4. PHY 参考源必须随 profile 自动选择：不得在 rev3.x 构建中强制旧版 PLL_F20M。
   Arduino 和托管 BSP 均使用自动选择路径。
5. 记录验证是静态的（源码/原理图）还是包含实体开发板测试。CI 通过只说明可以
   编译，不能单独证明引脚正确。

托管 BSP 保持已发布的 `3.0.1`，其中已经提供双地址、无引脚、轮询的触控约定，
不需要未发布的 `3.0.2`。本静态审计和编译成功均不能证明实体板卡的总线事务；
仍须在两种显示变体上完成有响应地址、坐标、抬起事件和轮询行为的 HIL 验证。
