# HelloApp — RAKOS demo user application

Minimal LVGL app that runs in **ota_1** (`0x400000`). Use it to test SD install and return-to-OS.

## Build

From the **RAK_os repo root** (not this folder):

```bash
cd D:/Git_code/RAK_os
pio run -e hello_app
```

Output:

- `.pio/build/hello_app/Hello.bin` — raw firmware
- `examples/HelloApp/dist/app.bin` — SD card package

## Test via SD card (recommended)

1. Flash **RAKOS** OS: `pio run -e rakos_os_local -t upload`
2. Copy `examples/HelloApp/dist/app.bin` to microSD:

   ```text
   /sdcard/Games/Hello/app.bin
   ```

3. Boot RAKOS → **Apps** → refresh → tap **Games / Hello**
4. Screen shows **Hello RAKOS** with an uptime counter.

## Test via USB (developer)

```bash
pio run -e hello_app -t upload
```

Then in RAKOS: **Apps** → **Run flashed app (ota_1)**.

## Return to RAKOS

- **BOOT** short press, or  
- **PWR** hold ~1.2 s  

## Files

| Path | Role |
|------|------|
| `src/main.cpp` | Demo UI + exit to OS |
| `partitions_app.csv` | App partition @ 0x400000 |
| `dist/app.bin` | Generated SD payload (after build) |
