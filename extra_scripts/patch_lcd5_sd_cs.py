"""Patch Arduino SD driver: CS on CH422G expander (pin -1 / 255) for Waveshare LCD-5."""
import os
from pathlib import Path

Import("env")

if "lcd5" not in env.subst("$PIOENV").lower():
    raise SystemExit()

MARKER = "RAKOS_LCD5_SD_CS_HOOK"
HOOK_BLOCK = f"""// {MARKER}
extern "C" bool rakos_sd_cs_external(void) __attribute__((weak));
extern "C" void rakos_sd_cs_set(int level) __attribute__((weak));

static void sd_cs_write(ardu_sdcard_t *card, int level) {{
  if (card->ssPin >= 48 && rakos_sd_cs_external && rakos_sd_cs_external()) {{
    rakos_sd_cs_set(level);
    return;
  }}
  digitalWrite(card->ssPin, level);
}}
"""

INIT_OLD = """  pinMode(card->ssPin, OUTPUT);
  digitalWrite(card->ssPin, HIGH);
  perimanSetPinBusExtraType(card->ssPin, "SD_SS");"""

INIT_NEW = f"""  if (card->ssPin >= 48 && rakos_sd_cs_external && rakos_sd_cs_external()) {{
    sd_cs_write(card, HIGH);
  }} else {{
    pinMode(card->ssPin, OUTPUT);
    digitalWrite(card->ssPin, HIGH);
    perimanSetPinBusExtraType(card->ssPin, "SD_SS");
  }}"""


def _sd_diskio_path():
    pkg = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
    return Path(pkg) / "libraries" / "SD" / "src" / "sd_diskio.cpp"


def _patch_sd_diskio():
    path = _sd_diskio_path()
    if not path.is_file():
        print(f"[patch_lcd5_sd_cs] skip: missing {path}")
        return

    text = path.read_text(encoding="utf-8")
    if MARKER in text:
        print("[patch_lcd5_sd_cs] already patched sd_diskio.cpp")
        return

    anchor = "static ardu_sdcard_t *s_cards[FF_VOLUMES] = {NULL};"
    if anchor not in text:
        print("[patch_lcd5_sd_cs] warn: anchor not found in sd_diskio.cpp")
        return
    text = text.replace(anchor, anchor + "\n\n" + HOOK_BLOCK, 1)

    for old in (
        "  digitalWrite(card->ssPin, HIGH);\n}",
        "  digitalWrite(card->ssPin, LOW);\n",
        "    digitalWrite(card->ssPin, HIGH);\n",
    ):
        if old == "  digitalWrite(card->ssPin, HIGH);\n}":
            text = text.replace(
                "void sdDeselectCard(uint8_t pdrv) {\n  ardu_sdcard_t *card = s_cards[pdrv];\n  digitalWrite(card->ssPin, HIGH);\n}",
                "void sdDeselectCard(uint8_t pdrv) {\n  ardu_sdcard_t *card = s_cards[pdrv];\n  sd_cs_write(card, HIGH);\n}",
                1,
            )
        elif old == "  digitalWrite(card->ssPin, LOW);\n":
            text = text.replace(
                "  digitalWrite(card->ssPin, LOW);\n  bool s = sdWait(pdrv, 500);",
                "  sd_cs_write(card, LOW);\n  bool s = sdWait(pdrv, 500);",
                1,
            )
            text = text.replace(
                "  digitalWrite(card->ssPin, LOW);\n  if (!sdWait(pdrv, 500)) {",
                "  sd_cs_write(card, LOW);\n  if (!sdWait(pdrv, 500)) {",
                1,
            )
        elif old == "    digitalWrite(card->ssPin, HIGH);\n":
            text = text.replace(
                "    digitalWrite(card->ssPin, HIGH);\n    return false;",
                "    sd_cs_write(card, HIGH);\n    return false;",
                1,
            )

    ff_init_old = "  // We send 20 bytes (160 clock cycles) to exceed the minimum requirement\n  digitalWrite(card->ssPin, HIGH);"
    ff_init_new = "  // We send 20 bytes (160 clock cycles) to exceed the minimum requirement\n  sd_cs_write(card, HIGH);"
    if ff_init_old in text:
        text = text.replace(ff_init_old, ff_init_new, 1)

    if INIT_OLD not in text:
        print("[patch_lcd5_sd_cs] warn: sdcard_init block not found")
        return
    text = text.replace(INIT_OLD, INIT_NEW, 1)

    path.write_text(text, encoding="utf-8")
    print(f"[patch_lcd5_sd_cs] patched {path}")


_patch_sd_diskio()
