# Competitive Analysis: RAKOS vs kodeOS & Alternatives

## Summary matrix

| Capability | kodeOS (Kode Dot) | RAKOS v0.2 | Notes |
|------------|-------------------|------------|-------|
| Target hardware | Kode Dot (integrated) | Waveshare AMOLED 1.8", LCD-5 | RAKOS is board-portable, not product-locked |
| Open source | Announced / partial | Full repo in tree | RAKOS fully buildable today |
| Launcher UI | Categories + icons + metadata | Grid/list + letter tiles | RAKOS lacks custom icons |
| App package | SD + name/icon/desc | SD + `app.bin` only | Major UX gap |
| Launch mechanism | Tap app (minimal reflash UX) | Copy bin → `ota_1` → reboot | RAKOS slower, single slot |
| USB Create App | Device UI after upload | Dev-only `pio upload` | kodeOS product feature |
| Maker mode | Countdown auto-run | NVS flag only, unwired | RAKOS stub |
| WiFi / time | Product integrated | `wifi.ini` + NTP | RAKOS recent addition |
| BLE | Yes | Basic advertise | RAKOS minimal |
| ESP-NOW | Advertised | No | Gap |
| App store / share | Planned / SD copy | SD copy only | Parity at file level only |
| Third-party apps | Community | Meshtastic experiment | RAKOS strength for mesh |
| IDE integration | Official board package | Custom `platformio.ini` | kodeOS smoother for beginners |
| Audio / IMU in OS | Central to product | BSP services, underused in UI | Similar hardware hooks |
| Battery / portable | Yes | Board-dependent | Waveshare AMOLED is USB-powered |

---

## kodeOS — strengths

1. **End-to-end maker UX** — Upload → Run / Create App on device without PC file copying.
2. **App identity** — Icons and descriptions make launcher usable with many apps.
3. **Perceived “no reflash”** — Switching apps feels instant compared to full firmware replace.
4. **Hardware + software bundle** — One SKU, one doc site, one board definition.
5. **Community narrative** — Share SD folders; future store and wireless share.

## kodeOS — limitations (from public info)

1. **Hardware lock-in** — Optimized for Kode Dot pin map and form factor.
2. **Documentation depth** — App bundle format and internals not fully public.
3. **Maturity** — Crowdfunding / early ecosystem; firmware OSS timeline varies.

---

## RAKOS — strengths

1. **Open, forkable codebase** — PlatformIO project with clear layers (`rakos_bsp`, `rakos_core`, …).
2. **Multi-board path** — AMOLED 1.8 + LCD-5 variants; Meshtastic as external app.
3. **Explicit OTA model** — `ota_0` / `ota_1` documented; easy to reason about for embedded devs.
4. **Policy hook** — `rakos_app_policy` gives deterministic return-to-OS behavior.
5. **No vendor cloud required** — SD + serial flash only.

## RAKOS — weaknesses (vs kodeOS)

1. **Install-style launch** — Every app switch may rewrite 0.5–1 MB+ to `ota_1`.
2. **No app metadata** — Folder name only; no PNG icon or description file.
3. **PC-centric install** — Copy `app.bin` to SD on computer; no device Create App.
4. **Maker mode not functional** — Setting exists without upload pipeline.
5. **SD reliability** — Board-specific SD path quirks (addressed in v0.2 but still sensitive).
6. **Onboarding** — No Arduino Board Manager entry; steeper for non-PlatformIO users.

---

## Other comparables (brief)

| Project | Relation to RAKOS |
|---------|-------------------|
| **Bare Arduino/ESP-IDF** | Single firmware; no launcher. RAKOS adds OS + app slot. |
| **M5Stack Launcher / UIFlow** | Vendor UI ecosystems; less hackable, different app model. |
| **MicroPython + filesystem** | Scripts on flash/SD; not binary OTA apps. |
| **Meshtastic firmware** | Full-stack mesh radio; RAKOS can host it as one `ota_1` app. |
| **Flipper Zero firmware** | Different MCU and app format; similar “app on SD” idea. |

---

## Strategic positioning

```text
                    High integration (product)
                              │
                    kodeOS ●  │
                              │
    Bare ESP32 ───────────────┼────────────── RAKOS (open launcher)
                              │
                    M5 / vendor ●
                              │
                    Low hackability ────────── High hackability
```

**RAKOS sweet spot**: Developers who already own **Waveshare (or similar) ESP32-S3 AMOLED** hardware and want a **self-hosted, kodeOS-inspired launcher** with Meshtastic/custom C++ apps—without buying Kode Dot.

**kodeOS sweet spot**: Buyers of **Kode Dot** who want polished out-of-box upload-to-app UX and community app sharing with minimal configuration.

---

## Recommended RAKOS roadmap (priority)

| Phase | Feature | Closes gap with |
|-------|---------|-----------------|
| P1 | `manifest.json` + PNG icon per app | kodeOS app identity |
| P1 | Stable SD + faster flash progress UX | kodeOS reliability perception |
| P2 | Category browser UI | kodeOS navigation |
| P2 | Maker USB protocol / Web flasher + Create App | kodeOS upload workflow |
| P3 | Optional run-from-SD or A/B app banks | kodeOS fast switch |
| P3 | ESP-NOW app beacon / share | kodeOS wireless share (future) |

---

## References

- [design.md](design.md) — RAKOS internals
- [kodeos-reference.md](kodeos-reference.md) — kodeOS public docs digest
- [CHANGELOG.md](../CHANGELOG.md) — what RAKOS has shipped
