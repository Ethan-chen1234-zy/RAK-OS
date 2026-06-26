"""NOP the legacy I2C conflict constructor when prebuilt IDF libs ignore sdkconfig."""
from pathlib import Path
from typing import Optional
import subprocess

Import("env")

if "lcd5" not in env.subst("$PIOENV").lower():
    raise SystemExit()


def _tool(name: str) -> str:
    for pkg in ("tool-xtensa-esp-elf", "toolchain-xtensa-esp-elf"):
        try:
            p = env.PioPlatform().get_package_dir(pkg)
            if p:
                cand = Path(p) / "bin" / name
                if cand.is_file():
                    return str(cand)
        except Exception:
            pass
    return name


def _va_to_file_offset(elf: Path, va: int) -> Optional[int]:
    out = subprocess.check_output(
        [_tool("xtensa-esp-elf-objdump"), "-h", str(elf)],
        text=True,
        errors="ignore",
    )
    for line in out.splitlines():
        line = line.strip()
        if not line or line.startswith("Idx"):
            continue
        parts = line.split()
        if len(parts) < 6:
            continue
        try:
            vma = int(parts[3], 16)
            off = int(parts[5], 16)
            size = int(parts[2], 16)
        except ValueError:
            continue
        if vma <= va < vma + size:
            return off + (va - vma)
    return None


def _patch_elf(elf_path: str) -> None:
    elf = Path(elf_path)
    if not elf.is_file():
        return

    sym_out = subprocess.check_output(
        [_tool("xtensa-esp-elf-nm"), "-n", str(elf)],
        text=True,
        errors="ignore",
    )
    addr = None
    for line in sym_out.splitlines():
        if "check_i2c_driver_conflict" in line:
            addr = int(line.split()[0], 16)
            break
    if addr is None:
        print("[patch_lcd5_i2c] check_i2c_driver_conflict not found (already disabled?)")
        return

    off = _va_to_file_offset(elf, addr)
    if off is None:
        print(f"[patch_lcd5_i2c] could not map VA 0x{addr:08x}")
        return

    data = bytearray(elf.read_bytes())
    # entry a1, 32 ; retw.n  (same prologue/epilogue as _Z4loopv in this ELF)
    patch = bytes([0x36, 0x41, 0x00, 0x1D, 0xF0, 0x00])
    if data[off : off + len(patch)] == patch:
        print("[patch_lcd5_i2c] already patched")
        return

    data[off : off + len(patch)] = patch
    elf.write_bytes(data)
    print(f"[patch_lcd5_i2c] patched check_i2c_driver_conflict @0x{addr:08x}")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", lambda target, source, env: _patch_elf(str(target[0])))
# Regenerate .bin from patched ELF (PlatformIO runs esptool after elf post-actions).
env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.bin",
    lambda target, source, env: subprocess.check_call(
        [
            env.subst("$PYTHONEXE"),
            "-m",
            "esptool",
            "--chip",
            "esp32s3",
            "elf2image",
            "--flash-mode",
            "dio",
            "--flash-freq",
            "80m",
            "--flash-size",
            "16MB",
            "-o",
            env.subst("$BUILD_DIR/${PROGNAME}.bin"),
            env.subst("$BUILD_DIR/${PROGNAME}.elf"),
        ]
    ),
)
