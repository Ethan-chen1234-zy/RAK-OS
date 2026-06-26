"""Copy app firmware to examples/<App>/dist[_lcd5]/app.bin for SD packaging."""
import os
import shutil

Import("env")


def copy_app_bin(source, target, env):
    pioenv = env.subst("$PIOENV").lower()
    is_lcd5 = "lcd5" in pioenv
    example = env.GetProjectOption("custom_app_example", "HelloApp")
    sd_folder = env.GetProjectOption("custom_app_sd_folder", "Games/Hello")

    build_dir = env.subst("$BUILD_DIR")
    progname = env.subst("$PROGNAME")
    src = os.path.join(build_dir, progname + ".bin")

    dist_leaf = "dist_lcd5" if is_lcd5 else "dist"
    dst_dir = os.path.join(env.subst("$PROJECT_DIR"), "examples", example, dist_leaf)
    os.makedirs(dst_dir, exist_ok=True)
    dst = os.path.join(dst_dir, "app.bin")

    if not os.path.isfile(src):
        print("[copy_user_app] missing build output: %s" % src)
        return

    shutil.copy2(src, dst)
    print("[copy_user_app] SD package: %s" % dst)
    print("[copy_user_app] Copy to SD: /sdcard/%s/app.bin" % sd_folder)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_app_bin)
