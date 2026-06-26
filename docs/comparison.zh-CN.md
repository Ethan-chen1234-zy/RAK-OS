# 竞品分析：RAKOS vs kodeOS 及替代方案

[English](comparison.md)

## 总览对比

| 能力 | kodeOS (Kode Dot) | RAKOS v0.2 | 备注 |
|------|-------------------|------------|------|
| 目标硬件 | Kode Dot（一体产品） | Waveshare AMOLED 1.8"、LCD-5 | RAKOS 可移植，非产品锁定 |
| 开源 | 已宣布 / 部分 | 仓库完整可构建 | RAKOS 今日即可编译 |
| 启动器 UI | 分类 + 图标 + 元数据 | 网格/列表 + 字母磁贴 | RAKOS 无自定义图标 |
| 应用包 | SD + 名称/图标/描述 | SD + 仅 `app.bin` | 主要 UX 差距 |
| 启动机制 | 点击应用（低感知重刷） | 复制 bin → `ota_1` → 重启 | RAKOS 较慢，单槽位 |
| USB 创建应用 | 上传后设备 UI | 仅开发 `pio upload` | kodeOS 产品特性 |
| Maker 模式 | 倒计时自动运行 | 仅 NVS 标志，未接线 | RAKOS 占位 |
| WiFi / 时间 | 产品集成 | `wifi.ini` + NTP | RAKOS 新增 |
| BLE | 有 | 基础广播 | RAKOS 较简 |
| ESP-NOW | 宣传有 | 无 | 差距 |
| 应用商店 / 分享 | 规划 / SD 复制 | 仅 SD 复制 | 文件级勉强对等 |
| 第三方应用 | 社区 | Meshtastic 实验 | RAKOS 网状网络优势 |
| IDE 集成 | 官方板型包 | 自定义 `platformio.ini` | kodeOS 对新手更顺 |
| OS 中音频 / IMU | 产品核心 | BSP 已有，UI 用得少 | 硬件钩子类似 |
| 电池 / 便携 | 有 | 视板卡而定 | Waveshare AMOLED 为 USB 供电 |

---

## kodeOS — 优势

1. **端到端创客体验** — 上传 → Run / Create App，无需 PC 拷文件。
2. **应用身份** — 图标与描述使启动器在应用多时仍可用。
3. **感知「无需重刷」** — 切换应用比整片换固件更即时。
4. **软硬一体** — 单一 SKU、文档站、板型定义。
5. **社区叙事** — 分享 SD 文件夹；未来商店与无线分享。

## kodeOS — 局限（据公开信息）

1. **硬件绑定** — 针对 Kode Dot 引脚与外形优化。
2. **文档深度** — 应用包格式与内部实现未完全公开。
3. **成熟度** — 众筹 / 早期生态；固件开源时间线不定。

---

## RAKOS — 优势

1. **开放可 fork 代码** — PlatformIO 项目，分层清晰（`rakos_bsp`、`rakos_core` 等）。
2. **多板路径** — AMOLED 1.8 + LCD-5；Meshtastic 作外部应用。
3. **显式 OTA 模型** — `ota_0` / `ota_1` 有文档，嵌入式开发者易理解。
4. **策略钩子** — `rakos_app_policy` 保证确定性返回 OS。
5. **无需厂商云** — 仅 SD + 串口烧录。

## RAKOS — 劣势（相对 kodeOS）

1. **安装式启动** — 每次切换可能重写 0.5–1 MB+ 到 `ota_1`。
2. **无应用元数据** — 仅文件夹名；无 PNG 图标或描述文件。
3. **以 PC 为中心安装** — 电脑上复制 `app.bin`；无设备端 Create App。
4. **Maker 模式未生效** — 有设置无上传管线。
5. **SD 可靠性** — 板级路径差异（v0.2 已修但仍敏感）。
6. **上手门槛** — 无 Arduino 板管理器入口；非 PlatformIO 用户较陡。

---

## 其他可比方案（简述）

| 项目 | 与 RAKOS 关系 |
|------|----------------|
| **裸 Arduino/ESP-IDF** | 单一固件；无启动器。RAKOS 增加 OS + 应用槽。 |
| **M5Stack Launcher / UIFlow** | 厂商 UI 生态；可 hack 性较低，应用模型不同。 |
| **MicroPython + 文件系统** | Flash/SD 上脚本；非二进制 OTA 应用。 |
| **Meshtastic 固件** | 完整网状电台栈；RAKOS 可将其作为 `ota_1` 应用宿主。 |
| **Flipper Zero 固件** | 不同 MCU 与应用格式；类似「SD 上的应用」理念。 |

---

## 战略定位

```text
                    高集成度（产品化）
                              │
                    kodeOS ●  │
                              │
    裸 ESP32 ───────────────┼────────────── RAKOS（开放启动器）
                              │
                    M5 / 厂商 ●
                              │
                    低可 hack ────────── 高可 hack
```

**RAKOS 甜点**：已拥有 **Waveshare（或类似）ESP32-S3 AMOLED** 硬件、想要**自托管、kodeOS 风格启动器**并运行 Meshtastic/自定义 C++ 应用、且**不必购买 Kode Dot** 的开发者。

**kodeOS 甜点**：购买 **Kode Dot**、希望开箱即用的上传即应用体验与最少配置的社区应用分享的用户。

---

## RAKOS 建议路线图（优先级）

| 阶段 | 功能 | 缩小与 kodeOS 差距 |
|------|------|---------------------|
| P1 | 每应用 `manifest.json` + PNG 图标 | kodeOS 应用身份 |
| P1 | 稳定 SD + 更快刷写进度 UX | kodeOS 可靠性感知 |
| P2 | 分类浏览 UI | kodeOS 导航 |
| P2 | Maker USB 协议 / Web 烧录 + Create App | kodeOS 上传流程 |
| P3 | 可选从 SD 运行或 A/B 应用槽 | kodeOS 快速切换 |
| P3 | ESP-NOW 应用信标 / 分享 | kodeOS 无线分享（未来） |

---

## 参考

- [design.zh-CN.md](design.zh-CN.md) — RAKOS 内部设计
- [kodeos-reference.zh-CN.md](kodeos-reference.zh-CN.md) — kodeOS 公开文档摘要
- [CHANGELOG.md](../CHANGELOG.md) — RAKOS 已交付功能
