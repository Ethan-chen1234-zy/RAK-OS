"""Ensure legacy ESP Panel I2C can coexist with Arduino 3.3.x driver_ng on LCD-5."""
from pathlib import Path

Import("env")

if "lcd5" not in env.subst("$PIOENV").lower():
    raise SystemExit()


def _patch_sdkconfig_h():
    build_dir = Path(env.subst("$BUILD_DIR"))
    for rel in ("config/sdkconfig.h", "sdkconfig.h"):
        cfg = build_dir / rel
        if not cfg.is_file():
            continue
        text = cfg.read_text(encoding="utf-8", errors="ignore")
        needle = "#define CONFIG_I2C_SKIP_LEGACY_CONFLICT_CHECK 1"
        if needle in text:
            print(f"[patch_lcd5_i2c] already set in {cfg}")
            return True
        if "#define CONFIG_I2C_SKIP_LEGACY_CONFLICT_CHECK" in text:
            text = text.replace(
                "#define CONFIG_I2C_SKIP_LEGACY_CONFLICT_CHECK 0",
                needle,
            )
        else:
            text = text.replace("#pragma once", "#pragma once\n" + needle, 1)
        cfg.write_text(text, encoding="utf-8")
        print(f"[patch_lcd5_i2c] patched {cfg}")
        return True
    return False


# Run after cmake/sdkconfig generation and again right before link.
env.AddPreAction("buildprog", lambda *args, **kwargs: _patch_sdkconfig_h())
env.AddPreAction("$BUILD_DIR/${PROGNAME}.elf", lambda *args, **kwargs: _patch_sdkconfig_h())
