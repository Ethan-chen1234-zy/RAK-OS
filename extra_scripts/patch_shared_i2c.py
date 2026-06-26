"""Patch third-party libs to use a single shared Wire bus (ESP32 Arduino 3.x)."""
import os
from pathlib import Path

Import("env")

env_name = env.subst("$PIOENV")
base = os.path.join(env.subst("$PROJECT_LIBDEPS_DIR"), env_name)


def patch_file(rel_path, old, new, label):
    path = os.path.join(base, rel_path)
    if not os.path.isfile(path):
        print(f"[patch_shared_i2c] skip {label}: missing {path}")
        return
    text = Path(path).read_text(encoding="utf-8")
    if old not in text:
        if new.split("\n", 1)[0] in text:
            print(f"[patch_shared_i2c] already patched: {label}")
        else:
            print(f"[patch_shared_i2c] warn: pattern not found for {label}")
        return
    Path(path).write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"[patch_shared_i2c] patched {label}")


# XPowersLib: do not re-init Wire on ESP32 (app owns Wire.begin).
for old in (
    """#elif defined(ARDUINO_ARCH_ESP32)
        __wire->begin(__sda, __scl);""",
    """#elif defined(ARDUINO_ARCH_ESP32)
            __wire->begin(__sda, __scl);""",
):
    patch_file(
        os.path.join("XPowersLib", "src", "XPowersCommon.tpp"),
        old,
        old.replace(
            "__wire->begin(__sda, __scl);",
            "// RAKOS: shared I2C bus\n        __wire->setClock(400000);",
        ),
        "XPowersLib",
    )

# SensorLib: same rule for IMU/PMIC helpers using SensorCommI2C.
patch_file(
    os.path.join("SensorLib", "src", "platform", "arduino", "SensorCommArduino_I2C.hpp"),
    """    bool init() override
    {
        setPins();
        wire.begin();
        return true;
    }""",
    """    bool init() override
    {
        setPins();
        // RAKOS: shared I2C bus — Wire.begin() is called once in firmware setup.
        wire.setClock(400000);
        return true;
    }""",
    "SensorLib I2C",
)
