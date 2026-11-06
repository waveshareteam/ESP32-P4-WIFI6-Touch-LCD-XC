# 持续集成

[English](CI.md)

本仓库以 GitHub Actions 作为构建结论的权威来源。维护者可以在本地运行 Python
策略检查，但产品编译证据必须来自修改提交并推送后的 Actions。

## 工作流

| 工作流 | 用途 |
| --- | --- |
| [Documentation and repository policy](../.github/workflows/documentation.yml) | 始终可见的单元测试、Markdown、结构和路由策略检查 |
| [ESP-IDF projects](../.github/workflows/esp-idf-projects.yml) | 按变更选择 ESP32-P4 一方 ESP-IDF 示例构建 |
| [Arduino projects](../.github/workflows/arduino-projects.yml) | 按变更选择 ESP32-P4 一方 Arduino 示例构建 |
| [Maintained firmware](../.github/workflows/maintained-firmware.yml) | 按变更选择、仅限 P4 的 `firmware/brookesia` profile；与示例分离 |

三个工作流都会在每个 Pull Request 和推送到 `main` 时启动。产品工作流始终显示
路由结果，只有路由选中产品工程时才运行昂贵的构建任务。同一 Pull Request 出现
新提交时会取消过时的在途任务。Pull Request 使用精确的 head SHA，而不是 GitHub
生成的临时 merge commit。

## 静态策略门禁

策略工作流会运行：

```bash
python -m unittest discover -s .github/tests -p "test_*.py"
python .github/scripts/repo_self_check.py
python .github/scripts/audit_markdown.py . --all --config .github/scripts/markdown-audit-config.json
```

Pull Request 的 Markdown 检查会把 `--all` 换成 base SHA。仓库自检会验证必需文档
和工作流、生成物忽略规则、12 个直接 ESP-IDF 工程、10 个直接 Arduino 示例以及
示例索引。

对于每一组一方英文/中文 Markdown 配对，本地策略门禁要求两页顶部附近都含有互相
跳转的语言链接。内部链接在目标存在同语言配对时也必须保持读者语言；明确同时提供
两种语言目标的语言选择区块可以链接到两页。

## 完整变更路由

两个产品工作流和策略工作流共用一个分类器：

```bash
python .github/scripts/ci_change_router.py --base-ref <base-sha> --head-ref <head-sha>
```

分类器读取 `git diff --name-status -z --find-renames`，因此删除和重命名两端都会
保留原路径的构建影响。无效 ref 或意外空 diff 会作为操作失败退出，不能静默产生
绿色的零构建结果。策略工作流额外使用 `--strict-unknown`；新出现的未知路径即使
触发了保守的双全量矩阵，也必须先补充明确分类规则。

| 修改路径 | 产品路由 |
| --- | --- |
| `examples/esp-idf/<project>/**` 源码或配置 | 对应 ESP-IDF 工程 |
| `examples/arduino/examples/<sketch>/**` 源码 | 对应 Arduino 示例 |
| `examples/arduino/libraries/**` | 全部一方 Arduino 示例 |
| 产品工作流或共享路由器 | 对应的完整矩阵 |
| 一方 Markdown、`docs/**`、`assets/**`、`hardware/**`、纯策略文件 | 只运行策略检查 |
| `firmware/**` | 显示 `firmware_touched`，不推断为示例构建 |
| `.bin`、`.zip` 等固件/归档镜像 | 固件结果并标记必须显式发布审核 |
| 未分类的非文档路径 | 双完整矩阵并使严格策略失败 |

`firmware/brookesia` 是单独维护的交付/源码面，不会被当作另一个示例。路由选中时，
专用工作流使用 ESP-IDF `v5.5.5` 构建 `3_4c`（800 × 800）和 `4c`（720 × 720），各覆盖 `rev3_x` 与 `rev1_3`。Board Manager 生成板级配置，其中 `3_4c` 使用 `lcd_3_4_dsi_touch_c` amend。C6 Hosted 镜像是运行时依赖，不包含在这些产物中。

## ESP-IDF 矩阵

一方 ESP-IDF 工程必须同时包含 `CMakeLists.txt` 和 `main/`，并且是
`examples/esp-idf/` 的直接子目录。每个选中的工程使用：

| 设置 | 值 |
| --- | --- |
| ESP-IDF | `v5.5.5`、`v6.0.2` |
| Target | `esp32p4` |
| Silicon profile | `rev3_x` / ESP32-P4 revision >= 3.0（默认）；`rev1_3` 是显式兼容 overlay |
| PSRAM | 启用时，默认 `rev3_x` 配置使用 250 MHz，显式 `rev1_3` overlay 使用 200 MHz |
| GitHub Action | `espressif/esp-idf-ci-action@v1` |

对于已确认的 pre-v3 芯片，应使用隔离构建，并把 `sdkconfig.defaults.rev1_3`
作为最后一层 `SDKCONFIG_DEFAULTS`。工程 07 和 12 必须把
`sdkconfig.defaults.esp32p4` 保留在最终 profile 层之前。不要在两个 profile 间复用
生成的 `sdkconfig` 或二进制文件。

完整路由包含 40 个任务：01–06 工程使用共享/default 配置（6 × 2）；07–11 显示
工程显式构建 3.4C（800×800）和 4C（720×720）两种变体（5 × 2 × 2）；
`12_usb_extend_screen` 则为两种显示变体同时构建 `default` 和 CI 专用
`vendor-only` 配置（2 × 2 × 2）。vendor-only 与屏幕选择正交，同时关闭 HID 触控和
UAC 音频，并在依赖解析阶段省略托管 UAC 组件。

手动运行可以使用 `project=all`、工程名（如 `02_HelloWorld`）或完整工程路径。

## Arduino 矩阵

只有直接匹配 `examples/arduino/examples/<name>/<name>.ino` 的目录是一方示例，
第三方库内置示例不会被发现。每个选中的示例会针对两个显示型号构建：

| 设置 | 值 |
| --- | --- |
| Arduino-ESP32 | `3.3.11` |
| 开发板 | 通用 ESP32-P4、post-v3 silicon、32 MB Flash、启用 PSRAM |
| Silicon profile | CI 仅使用 `rev3_x` / `ChipVariant=postv3`；已确认的 pre-v3 硬件可显式改用 `ChipVariant=prev3` 构建，示例不会翻倍 |
| 显示变体 | 3.4C（`SCREEN_3INCH_4_DSI`）、4C（`SCREEN_4INCH_DSI`） |
| 内置库 | `examples/arduino/libraries/` |

完整路由为 10 个示例乘以 2 个显示变体，共 20 个构建任务。手动运行可以使用
`sketch=all`、示例名或完整示例路径。

## 可下载示例构建产物

产品构建成功后，Actions 会上传一个以工程/示例、显示变体、配置、框架和精确提交
短 SHA 命名的构建产物。可在工作流运行的 **Artifacts** 区域下载。通过验证的内容
包括 `manifest.json`、`SHA256SUMS`、`flash.sh`、`flash.bat`、由框架烧录计划引用
且保持安全相对路径的 `bin/` 文件，以及适合相应框架的诊断文件。

Arduino 打包器从完整 Arduino CLI `3.3.11` 构建目录读取 `flash_args` 和
`build.options.json`，只发布该次真实烧录计划列出的独立分段；不会猜测 offset，也
不会从 merged 镜像切分载荷。生成的确定性 ZIP 会被重新打开并与已验证目录核对。
路径不安全或重复、符号链接、元数据或载荷哈希/大小不符、分段重叠或越界、FQBN/core
不符合预期，以及 merged 或 whole-flash 镜像都会使 CI 失败。

Arduino ZIP 不包含原始 build options、expanded properties、ELF/map 调试元数据或
其他主机相关记录。公开内容只有经验证的 FQBN/core 身份，以及原始 build-options
的文件名、大小和 SHA-256。验证器会从私有构建目录重新计算原文件哈希，并拒绝公开
元数据中的用户名、绝对工作路径或工具缓存路径。

manifest 会记录完整产品提交 SHA、目标、FQBN、显示/分辨率、配置、框架、Flash
大小和设置、有序分段的 offset/size/hash、`segmented_payload_total`、可复制的分段
命令，以及精确 BSP 来源/版本/tree pin。Arduino 示例不链接 ESP-IDF managed BSP，
因此 BSP 被记录为 reference-only 检查点。烧录辅助脚本需要端口参数，可选波特率
参数，不会擦除 Flash，也不会写入填充后的整片镜像。验证与 HIL 步骤请参阅
[Arduino 分段烧录](ARDUINO_FLASHING_ZH.md)。

这些文件是构建诊断产物，不代表硬件验证。固件 profile 构建预期产生
[固件源码边界](FIRMWARE_ZH.md)中命名的 FactoryOnly combine bin 候选；编译或打包
不构成 HIL。Release 仍为手动/延后流程，单独维护的 `firmware/` 交付面保持独立。
