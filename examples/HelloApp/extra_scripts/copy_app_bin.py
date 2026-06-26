"""Copy firmware.bin to examples/HelloApp/dist/app.bin for SD card packaging."""
import os
import shutil

Import("env")


def copy_app_bin(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    progname = env.subst("$PROGNAME")
    src = os.path.join(build_dir, progname + ".bin")
    dst_dir = os.path.join(env.subst("$PROJECT_DIR"), "examples", "HelloApp", "dist")
    os.makedirs(dst_dir, exist_ok=True)
    dst = os.path.join(dst_dir, "app.bin")
    if os.path.isfile(src):
        shutil.copy2(src, dst)
        print("[hello_app] SD package ready: %s" % dst)
        print("[hello_app] Copy to: /sdcard/Games/Hello/app.bin")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_app_bin)
