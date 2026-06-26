# kodeOS Reference (Public Documentation)

This document summarizes **publicly available** information about **kodeOS** and the **Kode Dot** device, for RAKOS design alignment. It is not an official kodeOS spec; verify against primary sources.

## Primary sources

| Resource | URL |
|----------|-----|
| kodeOS — Apps | https://docs.kode.diy/en/kodeOS/apps |
| Kode Dot Quickstart | https://docs.kode.diy/en/kode-dot/quickstart |
| Product site | https://www.kode.diy/ |
| Kickstarter / press | [CNX Software overview](https://www.cnx-software.com/2025/11/11/kode-dot-an-easy-to-use-pocket-sized-battery-powered-esp32-s3-devkit/) |

Open-source status (per marketing): hardware and kodeOS intended to go open source post-crowdfunding; firmware repo availability may lag product docs.

---

## Product positioning

**Kode Dot** is a pocket ESP32-S3 devkit with integrated AMOLED, battery, IMU, microphone, speaker, buttons, GPIO header, magnetic expansion, and microSD. **kodeOS** is the on-device OS that turns uploaded sketches into **named, iconized applications** launchable from a touch UI—similar to a phone app drawer for maker projects.

### Stated goals (from docs & marketing)

1. **Code becomes apps** — Projects are saved with name, category, and icon on microSD.
2. **No full reflash per project** — Switch between stored apps from the launcher.
3. **Familiar IDEs** — Arduino IDE, PlatformIO, ESP-IDF upload over USB-C.
4. **Shareability** — Copy app folders from SD to share; future wireless / store.
5. **Maker-friendly** — Lower friction than juggling multiple dev boards and cables.

---

## kodeOS functional model

### App storage

- Apps live on **microSD**, organized by **category folders**.
- Default categories (docs): **General**, **Hacking**, **GPIO**, **USB**, **Games** (user may add more).
- Each app has metadata: **name**, **icon**, **description** (exact on-disk format is not fully documented in public pages).

### Upload workflow

1. Connect Kode Dot via USB; enter **upload mode** on device (e.g. swipe gesture to upload menu).
2. Select **Kode Dot** board in Arduino IDE / PlatformIO.
3. Upload sketch from PC.
4. On device: choose **Run** (execute once) or **Create App** (save to SD with category).

### Maker vs managed behavior

- **Maker mode** (when enabled): after upload, a **3-second countdown** runs before auto-executing code; user can cancel.
- **Managed mode**: different auto-run policy (details in device settings).

### Running apps

- Open **Applications** menu → pick category → tap app icon.
- Returning to OS does not require reflashing the entire device firmware.

### Connectivity (hardware + OS)

- Wi-Fi, Bluetooth (Kode Dot specs; some variants mention ESP32-C6 for RF).
- **ESP-NOW** listed in product materials.
- Future: download apps from store, share between devices wirelessly.

---

## Kode Dot hardware (reference)

| Feature | Typical spec (from public materials) |
|---------|--------------------------------------|
| MCU | ESP32-S3 class |
| Flash / PSRAM | Up to 32 MB flash, 8 MB PSRAM (product variant) |
| Display | ~2.13" AMOLED, touch |
| Storage | microSD |
| Sensors | 6-axis IMU + magnetometer (9-axis) |
| Audio | Microphone + speaker |
| Power | Battery + USB-C |
| I/O | GPIO header, magnetic connector, RGB LED |

*Waveshare AMOLED 1.8" (RAKOS primary target) differs: no battery, different IMU, ES8311 audio, SDMMC 1-bit, no magnetometer.*

---

## kodeOS design principles (inferred)

| Principle | Description |
|-----------|-------------|
| **App-centric UX** | User thinks in “apps”, not “firmware images”. |
| **SD as package manager** | Distribution and sharing via files on card. |
| **IDE-transparent** | Same upload habit as bare ESP32, extra step only when saving as app. |
| **Guardrails for makers** | Countdown / modes prevent accidental runs during development. |
| **Ecosystem growth** | Categories, sharing, and store as community hooks. |

---

## What public docs do *not* fully specify

- Exact on-disk app bundle format (binary layout, manifest schema, icon size)
- Whether user code runs from SD directly or is copied to an OTA partition internally
- Full API for apps to call OS services (backlight, WiFi, etc.)
- Complete open-source repository layout and license as of any given date

RAKOS implementers should treat these as **research gaps** when porting kodeOS-compatible packages.

---

## Search keywords for further research

- `site:docs.kode.diy kodeOS`
- `kodeOS create app microSD`
- `Kode Dot maker mode countdown`
- `kodeOS open source GitHub`

---

## Relation to RAKOS

RAKOS adopts the **category + SD + launcher** metaphor but currently uses a **simpler package format** (`app.bin` only) and **install-to-ota_1** launch path. See [comparison.md](comparison.md) for gap analysis.
