# RAKOS Project Overview

> One-page intro: what it is, why it exists, how it works, where it's going. Good for demos and quick onboarding.

[中文](overview.zh-CN.md) · Details: [design.md](design.md)

---

## In one sentence

**RAKOS is a "mini phone home screen" on an ESP32-S3 touch board** — the OS stays in flash, apps live on SD card, tap an icon to run, hold BOOT to return home.

---

## What problem does it solve?

On ESP32, switching programs usually means **reflashing the entire firmware**. A board with a display could act like a small gadget, but you're always plugging in USB and rebuilding.

RAKOS takes inspiration from [kodeOS](https://docs.kode.diy/):

| Before | With RAKOS |
|--------|------------|
| One project = one board | One board = OS + many apps |
| Change app → full flash erase | Pick app from SD → flash to app slot → run |
| No shared launcher | LVGL desktop: Home, Apps grid, Setup |

**Target users**: Makers with a Waveshare AMOLED 1.8" (or LCD-5) who already use PlatformIO/Arduino and C++.

---

## Design in three diagrams

### 1. Split flash

```text
┌─────────────────────────────────────┐
│  ota_0  @ 0x20000   ← RAKOS OS      │  permanent
├─────────────────────────────────────┤
│  ota_1  @ 0x400000  ← running app   │  updated when switching apps
├─────────────────────────────────────┤
│  storage (SPIFFS)   ← small config  │
└─────────────────────────────────────┘
         microSD
         Games/Hello/app.bin
         wifi.ini
```

**Principle**: OS and apps are **physically separate**; a broken app doesn't brick the launcher.

### 2. SD card as app folder

Copy `app.bin` packages to category folders → **SD Scan** in Apps → tap tile → OS writes `ota_1` and reboots.

### 3. Simple layers

```text
  Launcher UI (LVGL)
        ↓
  rakos_apps / rakos_core / rakos_bsp
        ↓
  ESP-IDF / hardware
```

Not a full OS — a **practical launcher + BSP** for embedded.

---

## Demo flow (~5 minutes)

```text
① Flash RAKOS once:  pio run -e rakos_os_appui -t upload
② TF card: Games/Hello/app.bin (+ optional wifi.ini)
③ Power on → Apps → SD Scan → tap Hello
④ Run demo app
⑤ Hold BOOT ~1.5 s → back to RAKOS
```

---

## Current (v0.2)

Dual OTA, icon-grid launcher, SD categories, Hello/Clock/Btn/Meter/Slider demos, WiFi + NTP clock, LCD-5 variant, experimental Meshtastic app host.

---

## Direction

| Now | Near term | Later |
|-----|-----------|-------|
| `app.bin` only | `manifest.json` + icons | On-device Create App |
| Flash to `ota_1` | Category browser | Faster switch / multi-bank |
| PC → SD copy | Flash progress UX | Wireless share / store |

See [comparison.md](comparison.md).

---

## vs kodeOS

Same **apps-on-SD** idea; RAKOS is an **open, forkable launcher** for Waveshare-class boards rather than a bundled Kode Dot product.

---

**Version**: `0.2.0-dev` · **Primary board**: Waveshare ESP32-S3-Touch-AMOLED-1.8
