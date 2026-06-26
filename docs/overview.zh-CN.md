# RAKOS 项目简介

> 一页纸说明：这是什么、为什么做、怎么工作、往哪走。适合演示、分享或快速 onboarding。

[English](overview.md) · 详细设计见 [design.zh-CN.md](design.zh-CN.md)

---

## 一句话

**RAKOS 是一块 ESP32-S3 触摸屏开发板上的「小手机桌面」**——系统常驻 Flash，应用放在 SD 卡，点图标就能运行，长按 BOOT 回到桌面。

---

## 解决什么问题？

传统 ESP32 开发习惯是：**每换一个程序，就整片重刷固件**。  
一块带屏的板子本来可以当「小设备」用，却总要插线、选环境、等编译。

RAKOS 借鉴 [kodeOS](https://docs.kode.diy/) 的思路：

| 以前 | 用 RAKOS |
|------|----------|
| 一个工程 = 一块板 | 一块板 = 系统 + 多个应用 |
| 换程序要重刷整片 Flash | 从 SD 选应用，刷到专用槽位后启动 |
| 没有统一桌面 | LVGL 启动器：主页、应用网格、设置 |

**目标用户**：已有 Waveshare AMOLED 1.8"（或 LCD-5）板子、用 PlatformIO/Arduino 写 C++ 的创客和嵌入式开发者。

---

## 设计思路（三张图）

### 1. Flash 一分为二

```text
┌─────────────────────────────────────┐
│  ota_0  @ 0x20000   ← RAKOS 系统    │  常驻，很少动
├─────────────────────────────────────┤
│  ota_1  @ 0x400000  ← 当前运行的应用 │  切换应用时更新
├─────────────────────────────────────┤
│  storage (SPIFFS)   ← 系统小配置     │
└─────────────────────────────────────┘
         microSD（TF 卡）
         Games/Hello/app.bin
         Games/Clock/app.bin
         wifi.ini
```

**原则**：系统和应用**物理分离**，互不覆盖；应用坏了也不影响重新进桌面。

### 2. SD 卡当「应用商店文件夹」

```text
TF 卡根目录/
├── wifi.ini              # 可选：自动连 WiFi
├── Games/
│   ├── Hello/app.bin
│   ├── Clock/app.bin
│   └── Btn/app.bin
└── General/
    └── ...
```

每个 `app.bin` 是一个独立 PlatformIO 工程编译出的固件包。  
复制到 SD → 在 **应用** 页 **SD Scan** → 点图标 → 系统写入 `ota_1` 并重启运行。

### 3. 软件分层（保持简单）

```text
  启动器 UI (LVGL)     ← 用户看到的桌面
        ↓
  rakos_apps           ← 扫 SD、登记应用
  rakos_core           ← 选 ota_0/ota_1、重启
  rakos_bsp            ← 屏、触摸、SD、WiFi、音频
        ↓
  ESP-IDF / 硬件
```

不追求完整 OS，只做**嵌入式能落地的启动器 + BSP**。

---

## 典型使用流程（可现场演示）

```text
① 烧录 RAKOS（一次性）
   pio run -e rakos_os_appui -t upload

② 准备 TF 卡
   复制 Games/Hello/app.bin
   可选：wifi.ini（ssid / password）

③ 上电
   主页显示时间（联网后）
   「应用」页 → SD Scan → 点 Hello

④ 运行 Hello 演示
   屏幕显示 LVGL 界面

⑤ 返回桌面
   长按 BOOT ~1.5 秒 → 回到 RAKOS
```

**5 分钟演示清单**：系统桌面 → 扫 SD → 起一个应用 → 回桌面 →（可选）换 Clock/Btn 应用。

---

## 当前能做什么（v0.2）

- 双 OTA：系统 / 应用分区
- 图标网格启动器（`rakos_os_appui`）
- SD 分类扫描（Games、General 等）
- 演示应用：Hello、Clock、Btn、Meter、Slider
- WiFi + NTP 时钟（`wifi.ini` 或设置页）
- LCD-5 800×480 变体
- 实验：Meshtastic 作为外部 `app.bin` 宿主

---

## 方向（路线图摘要）

```text
  现在                    近期                     更远
  ────                    ────                     ────
  app.bin 包              manifest + 图标          设备端「创建应用」
  刷 ota_1 后启动         分类浏览 UI              更快切换 / 多槽
  PC 拷 SD                刷写进度 UX              无线分享 / 商店
```

与 kodeOS 的差距与优先级见 [comparison.zh-CN.md](comparison.zh-CN.md)。

---

## 和 kodeOS 的关系

| | kodeOS (Kode Dot) | RAKOS |
|---|-------------------|--------|
| 定位 | 商业一体硬件 + 官方 OS | 开源启动器，适配 Waveshare 等板 |
| 理念 | 代码变应用、SD 分发 | **相同理念** |
| 体验 | 上传后设备上「创建应用」 | 目前 PC 编译 + 拷 SD（更极客） |
| 代码 | 部分开源 / 产品绑定 | **全仓库可构建、可 fork** |

RAKOS 不是 kodeOS 克隆，而是**在已有硬件上实现类似体验的开源方案**。

---

## 进一步阅读

| 文档 | 适合谁 |
|------|--------|
| [README.zh-CN.md](../README.zh-CN.md) | 编译、烧录、应用列表 |
| [design.zh-CN.md](design.zh-CN.md) | 架构与启动流程细节 |
| [kodeos-reference.zh-CN.md](kodeos-reference.zh-CN.md) | kodeOS 公开资料整理 |
| [comparison.zh-CN.md](comparison.zh-CN.md) | 竞品与路线图 |
| [CHANGELOG.md](../CHANGELOG.md) | 版本变更 |

---

**版本**：`0.2.0-dev` · **主硬件**：Waveshare ESP32-S3-Touch-AMOLED-1.8
