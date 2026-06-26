# RAKOS 与 kodeOS 设计差距分析

本文基于当前工程源码、`README`、`docs/`、`CHANGELOG.md` 与 `ARCHITECTURE.md` 梳理 RAKOS 相对 kodeOS 的设计一致性、不足与优化方向。kodeOS 侧仅参考公开资料，公开资料未说明的实现细节不作为确定事实。

## 结论

RAKOS 与 kodeOS 的核心理念是基本一致的：都把 ESP32-S3 固件项目抽象成可在设备 UI 中启动的“应用”，都以 microSD 作为应用分发介质，并希望保留 Arduino / PlatformIO 这类熟悉工具链。

当前差异主要不在方向，而在产品化闭环：RAKOS 已具备“启动器 + SD 扫描 + `ota_1` 用户应用槽 + 返回 OS”的技术骨架，但还没有形成 kodeOS 宣称的“上传后 Run / Create App、具名图标应用、快速切换、分享生态”的完整体验。因此 RAKOS 更像开放、可移植、开发者友好的 kodeOS 风格运行时；kodeOS 更像软硬一体、面向普通创客的端到端产品。

## 当前 RAKOS 设计现状

- OS 与应用分离：RAKOS 运行在 `ota_0`，用户应用安装到 `ota_1`。
- 应用发现：`AppRegistry` 扫描 SD 根目录下 `General`、`Games`、`GPIO`、`USB`、`Hacking` 等分类目录，寻找每个应用文件夹中的 `app.bin`。
- 启动机制：用户点击应用后，启动器从 SD 读取 `app.bin`，擦写 `ota_1`，设置下次启动分区并重启。
- 返回 OS：示例应用通过 `rakos::AppRuntime` 与 `rakos_app_policy` 支持 BOOT 长按或屏幕按钮回到 `ota_0`。
- UI 与 BSP：已实现 LVGL Home / Apps / Setup / Info、应用网格、WiFi/NTP、BLE 广播、SD 热插拔轮询、AMOLED 与 LCD-5 两条板级路径。
- 配置状态：`OsConfig` 保存 Maker / Managed 与默认启动槽位，`RadioService` 保存 WiFi / BLE 配置。

## 与 kodeOS 一致的设计理念

| 理念 | kodeOS | RAKOS 当前实现 | 一致性 |
|------|--------|----------------|--------|
| 应用中心 | 用户从 Applications 菜单启动项目 | Apps 页扫描并启动 SD 应用 | 一致 |
| SD 作为包分发介质 | 应用按分类保存在 microSD | SD 分类目录 + `app.bin` | 基本一致 |
| 保留熟悉工具链 | Arduino / PlatformIO / ESP-IDF 上传 | PlatformIO 构建 OS 与用户应用 | 一致，但 RAKOS 更偏开发者 |
| OS / 用户代码隔离 | 上传代码可作为应用运行 | `ota_0` OS + `ota_1` 单应用槽 | 一致 |
| 可返回启动器 | 从应用回到 OS | `rakos_app_policy` + BOOT / 屏幕按钮 | 一致 |
| 创客护栏 | Maker / Managed、倒计时运行 | 仅有 NVS 与设置入口 | 方向一致，行为未闭环 |
| 生态扩展 | 分类、图标、分享、商店 | 分类目录与 Meshtastic 实验应用 | 方向一致，生态能力较弱 |

## 主要不足

### 1. 应用包语义过薄

RAKOS 当前只识别 `app.bin` 与文件夹名。相比 kodeOS 的“名称、图标、描述、分类”应用身份，缺少 `manifest.json`、图标、描述、版本、目标板型、最小 OS 版本、权限/能力声明、作者信息和校验信息。

影响是应用数量一多，启动器只能显示字母磁贴和截断名称，无法做兼容性过滤、版本提示、详情页、分享索引或未来商店。

优化建议：

- P1：定义 `manifest.json` schema，最少包含 `name`、`category`、`version`、`board`、`entry`、`icon`、`description`、`min_rakos`。
- P1：保持向后兼容，若无 manifest 则继续用文件夹名 + `app.bin`。
- P2：增加 PNG / LVGL raw icon 解析与应用详情页。
- P2：加入 SHA-256 / size 校验，安装前检查文件完整性与目标板型。

### 2. 启动路径仍是“安装式切换”

当前每次从 SD 启动应用都会擦写 `ota_1`，并且擦写的是整个 `ota_1` 分区。这比整片重刷友好，但仍不是 kodeOS 宣称的低感知应用切换体验。

影响包括切换慢、磨损更高、SD 必须稳定在线、失败恢复信息有限；即使用户重复打开同一个应用，也会经历完整擦写。

优化建议：

- P1：记录已安装应用的 manifest/hash，重复启动同一镜像时跳过重刷，直接启动 `ota_1`。
- P1：只擦除镜像覆盖范围按扇区对齐后的区域，而不是固定擦完整 8 MB 槽位。
- P1：写入后使用 ESP-IDF 镜像校验能力验证 image header、segment 与 hash，再允许设置启动槽。
- P2：安装进度显示“校验 / 擦除 / 写入 / 完成”，错误页展示具体失败阶段。
- P3：在 16 MB Flash 约束下评估双应用槽、小型缓存槽或压缩包方案；若目标硬件升级到 32 MB，再考虑多应用 bank。

### 3. USB Create App / Maker 模式没有打通

工程里已有 `OsMode::Maker`、`OsMode::Managed` 和默认启动槽位设置，但当前没有上传捕获、倒计时运行、Run / Create App、从 `ota_1` 导出到 SD 应用包等流程。PlatformIO 的 `hello_app -t upload` 仍是开发者直接写 `0x400000`。

这意味着 RAKOS 对熟悉 PlatformIO 的用户可用，但还不具备 kodeOS 面向新手的“上传后在设备上处理应用”的闭环。

优化建议：

- P1：让 `default_boot_subtype` 真正参与启动策略，明确 Managed / Maker 在 boot 时的行为。
- P2：定义 USB 上传工作流：上传到 `ota_1` 后，OS 检测新镜像，显示 Run / Create App。
- P2：Create App 将当前 `ota_1` 镜像复制到 SD 指定分类，并生成 manifest。
- P2：Maker 模式增加 3 秒倒计时、取消按钮和默认动作。
- P3：提供 Web flasher 或简化脚本，降低非 PlatformIO 用户门槛。

### 4. UI 仍缺少分类优先与应用管理能力

应用网格已经比列表更接近 kodeOS，但当前仍是把所有可运行应用平铺分页，分类只是 `AppEntry` 字段，不是主要导航结构。系统操作 Flash / SD Scan 也与应用入口混在 Apps 页顶端。

优化建议：

- P1：Apps 页先显示分类，再进入分类应用列表；小屏 AMOLED 可保留“最近 / 收藏”快捷区。
- P1：增加应用详情页，展示描述、版本、大小、目标板、安装状态。
- P2：增加最近运行、收藏、卸载/清理 `ota_1`、重新扫描日志。
- P2：统一 AMOLED 与 LCD-5 的交互布局，减少按键返回和屏幕返回的差异感。

### 5. OS 服务 API 尚不稳定

用户应用目前通过链接 `rakos_bsp`、`AppRuntime` 和策略库获得显示、触摸与返回 OS 能力，本质上仍是“每个 app 自带完整固件 + BSP”。这符合 ESP32 无 MMU 的现实，但还不是稳定的 OS service 模型。

优化建议：

- P1：文档化用户应用 ABI / API：必须链接哪些库、必须设置哪些 flags、如何返回 OS、支持哪些板型。
- P2：提供 `rakos_app.h` 聚合头和最小应用模板，减少每个示例重复配置。
- P2：定义能力约定，例如 display、touch、audio、imu、wifi 是否由 app 独占，OS 不在后台运行。
- P3：探索轻量跨重启状态传递，例如上一应用、退出原因、错误码、安装结果。

### 6. 无线与分享能力较弱

kodeOS 公开资料提到 WiFi、蓝牙、ESP-NOW、未来商店/无线分享。RAKOS 当前有 WiFi STA、NTP 与基础 BLE 广播，但没有 ESP-NOW 应用发现/分享，也没有应用索引或商店格式。

优化建议：

- P2：先定义离线分享包格式：一个应用文件夹即可复制，manifest + icon + app.bin 完整自描述。
- P3：增加 ESP-NOW 或 BLE 广播应用摘要，只做发现，不直接自动安装。
- P3：实现“接收应用”前的签名/哈希确认，避免无线分发带来安全风险。

### 7. 安全与可靠性需要补强

当前 WiFi 凭据以明文保存在 SD `wifi.ini` 或 NVS；应用安装缺少 manifest 校验、签名、兼容性检查；工程内也没有看到独立的 RAKOS 单元测试或 CI 工作流。

优化建议：

- P1：安装前做镜像校验、大小检查、板型检查与错误提示。
- P1：增加 AppRegistry / manifest parser 的主机侧单元测试。
- P2：提供测试 SD 镜像样例，覆盖空卡、坏 manifest、无 app.bin、大文件、错误板型。
- P2：WiFi 密码继续允许明文 SD 配置，但文档标注风险；长期可迁移到 NVS 加密或仅设备端输入。
- P3：应用签名作为可选安全层，至少支持“可信开发者 / 未验证应用”提示。

### 8. 板级可移植性仍依赖本地环境

RAKOS 已支持 AMOLED 与 LCD-5，但 `platformio.ini` 中存在本地 `symlink://D:/...` 依赖路径，LCD-5 与 AMOLED 使用不同 LVGL 版本和显示栈，示例应用也未完全同步。

优化建议：

- P1：把本地 symlink 依赖替换为可下载包、git submodule 或清晰的 vendor 导入脚本。
- P1：把板级差异收敛到 board profile，避免应用环境重复大量 build flags。
- P2：补齐 LCD-5 的 Btn / Meter / Slider 示例，保证多板能力不是文档能力。
- P2：生成应用模板时根据目标板自动选择 BSP、LVGL 配置和退出方式。

## 优先级路线图

| 优先级 | 工作项 | 价值 |
|--------|--------|------|
| P1 | `manifest.json` + 兼容旧 `app.bin` 扫描 | 建立应用身份，是后续 UI、分享、校验的基础 |
| P1 | 安装 hash 缓存 + 跳过重复重刷 | 立刻改善“每次都慢”的核心体验 |
| P1 | 镜像校验与更清晰错误页 | 降低坏 SD / 坏 app 带来的不可解释失败 |
| P1 | 应用开发模板与 API 文档 | 降低第三方应用接入成本 |
| P1 | 清理本地依赖路径 | 提升仓库可复现构建能力 |
| P2 | 分类优先 Apps UI + 应用详情页 | 更接近 kodeOS 应用抽屉体验 |
| P2 | Maker / Managed 启动策略生效 | 让现有设置从占位变成行为 |
| P2 | USB Run / Create App 工作流 | 补齐 kodeOS 最大产品化差距 |
| P2 | AppRegistry / manifest 单元测试 | 稳住应用生态基础 |
| P3 | 多应用 bank / 压缩安装 / 32 MB 方案 | 进一步缩短切换时间 |
| P3 | ESP-NOW / BLE 应用发现与分享 | 扩展社区分发能力 |
| P3 | 应用签名与可信来源提示 | 为分享和商店做安全准备 |

## 设计理念是否需要调整

不建议改变 RAKOS 的基本设计理念。`ota_0` OS + `ota_1` 用户应用槽是 ESP32-S3 上务实、可解释、容易调试的方案，也符合 RAKOS 面向开放硬件和自托管开发者的定位。

需要调整的是体验层目标：从“能从 SD 安装 app.bin 并启动”推进到“应用是有身份、有状态、可管理、可分享的包”。这与 kodeOS 的理念一致，但 RAKOS 可以保留自己的差异化：

- 继续保持开放 PlatformIO 工程，而不是绑定单一硬件产品。
- 优先做好可复现构建、清晰分区模型和第三方固件适配。
- 在产品化体验上逐步补齐 manifest、分类 UI、Create App 与快速切换，而不是一开始追求完整商店。

## 建议的下一步

1. 先实现 manifest 解析，但保留旧目录扫描逻辑。
2. 给安装器增加镜像 hash 缓存与重复启动跳过重刷。
3. 将 `OsConfig::default_boot_subtype` 和 Maker / Managed 模式接入启动策略。
4. 抽出标准用户应用模板，减少每个示例重复 `platformio.ini` 配置。
5. 建立最小 CI：至少编译 `rakos_os_appui`、`hello_app`、`clock_app`，并测试 manifest parser。

