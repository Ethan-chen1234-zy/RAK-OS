# kodeOS 参考资料（公开文档摘要）

[English](kodeos-reference.md)

本文汇总 **kodeOS** 与 **Kode Dot** 设备的**公开资料**，供 RAKOS 设计对齐参考。非 kodeOS 官方规范，请以原始来源为准。

## 主要来源

| 资源 | 链接 |
|------|------|
| kodeOS — Apps | https://docs.kode.diy/en/kodeOS/apps |
| Kode Dot 快速入门 | https://docs.kode.diy/en/kode-dot/quickstart |
| 产品官网 | https://www.kode.diy/ |
| Kickstarter / 媒体报道 | [CNX Software 综述](https://www.cnx-software.com/2025/11/11/kode-dot-an-easy-to-use-pocket-sized-battery-powered-esp32-s3-devkit/) |

开源状态（据宣传）：硬件与 kodeOS 计划在众筹后开源；固件仓库公开时间可能晚于产品文档。

---

## 产品定位

**Kode Dot** 是口袋型 ESP32-S3 开发板，集成 AMOLED、电池、IMU、麦克风、扬声器、按键、GPIO 排针、磁吸扩展与 microSD。**kodeOS** 是设备端 OS，将上传的 Sketch 变成可在触摸 UI 中启动的**具名、带图标的应用**——类似手机应用抽屉，面向创客项目。

### 宣称目标（文档与宣传）

1. **代码即应用** — 项目在 microSD 上保存名称、分类与图标。
2. **无需为每个项目整片重刷** — 从启动器在已存应用间切换。
3. **熟悉 IDE** — Arduino IDE、PlatformIO、ESP-IDF 经 USB-C 上传。
4. **可分享** — 复制 SD 应用文件夹分享；未来无线 / 商店。
5. **创客友好** — 比多块开发板与线缆来回切换更省事。

---

## kodeOS 功能模型

### 应用存储

- 应用存放在 **microSD**，按**分类文件夹**组织。
- 默认分类（文档）：**General**、**Hacking**、**GPIO**、**USB**、**Games**（用户可新增）。
- 每个应用含元数据：**名称**、**图标**、**描述**（公开页面未完全说明磁盘格式）。

### 上传流程

1. USB 连接 Kode Dot；设备进入**上传模式**（如滑动手势进入上传菜单）。
2. 在 Arduino IDE / PlatformIO 中选择 **Kode Dot** 板型。
3. 从 PC 上传 Sketch。
4. 设备上选择 **Run**（运行一次）或 **Create App**（按分类保存到 SD）。

### Maker 与 Managed 模式

- **Maker 模式**（启用时）：上传后 **3 秒倒计时**自动执行代码，用户可取消。
- **Managed 模式**：不同自动运行策略（详见设备设置）。

### 运行应用

- 打开 **Applications** 菜单 → 选分类 → 点击应用图标。
- 返回 OS 无需重刷整设备固件。

### 连接能力（硬件 + OS）

- Wi-Fi、蓝牙（Kode Dot 规格；部分变体提及 ESP32-C6 负责射频）。
- 产品材料列出 **ESP-NOW**。
- 未来：从商店下载应用、设备间无线分享。

---

## Kode Dot 硬件（参考）

| 特性 | 典型规格（公开资料） |
|------|----------------------|
| MCU | ESP32-S3 级 |
| Flash / PSRAM | 最高 32 MB Flash、8 MB PSRAM（视型号） |
| 显示 | ~2.13" AMOLED，触摸 |
| 存储 | microSD |
| 传感器 | 6 轴 IMU + 磁力计（9 轴） |
| 音频 | 麦克风 + 扬声器 |
| 供电 | 电池 + USB-C |
| I/O | GPIO 排针、磁吸接口、RGB LED |

*Waveshare AMOLED 1.8"（RAKOS 主目标）差异：无电池、不同 IMU、ES8311 音频、SDMMC 1-bit、无磁力计。*

---

## kodeOS 设计原则（推断）

| 原则 | 说明 |
|------|------|
| **应用中心 UX** | 用户以「应用」而非「固件镜像」思考。 |
| **SD 即包管理器** | 通过卡上文件分发与分享。 |
| **IDE 透明** | 上传习惯与裸 ESP32 相同，仅保存为应用时多一步。 |
| **创客护栏** | 倒计时 / 模式防止开发时误运行。 |
| **生态增长** | 分类、分享、商店作为社区钩子。 |

---

## 公开文档*未*完全说明的内容

- 磁盘应用包的确切格式（二进制布局、manifest 模式、图标尺寸）
- 用户代码是否直接从 SD 运行，还是内部复制到 OTA 分区
- 应用调用 OS 服务（背光、WiFi 等）的完整 API
- 任意时点的完整开源仓库结构与许可证

移植 kodeOS 兼容包时，RAKOS 实现者应将这些视为**待研究空白**。

---

## 进一步检索关键词

- `site:docs.kode.diy kodeOS`
- `kodeOS create app microSD`
- `Kode Dot maker mode countdown`
- `kodeOS open source GitHub`

---

## 与 RAKOS 的关系

RAKOS 采用 **分类 + SD + 启动器** 隐喻，但当前使用**更简包格式**（仅 `app.bin`）和 **安装到 ota_1** 的启动路径。差距分析见 [comparison.zh-CN.md](comparison.zh-CN.md)。
