# ESP32-P4-WIFI6-Touch-LCD-4C 固件

[English](README.md) | [简体中文](README_ZH.md)

[返回固件列表](../README_ZH.md)

ESP32-P4-WIFI6-Touch-LCD-4C 的出厂固件。BIN 是从 `0x0` 烧录的合并镜像，文件名中的日期即版本。

| 文件 | 用途 |
| --- | --- |
| [ESP32-P4-WIFI6-Touch-LCD-4C_FactoryOnly_20261106.bin](ESP32-P4-WIFI6-Touch-LCD-4C_FactoryOnly_20261106.bin) | 出厂固件，恢复开箱演示 |

在本目录运行以下命令，将 `<PORT>` 替换为开发板对应的串口。整片擦除会清除已有设置和用户数据。

```bash
python -m pip install esptool
esptool.py --chip esp32p4 --port <PORT> erase_flash
esptool.py --chip esp32p4 --port <PORT> write_flash 0x0 ESP32-P4-WIFI6-Touch-LCD-4C_FactoryOnly_20261106.bin
```
