Import("env")


def _skip_lvgl_arm_asm(node, *args, **kwargs):
    path = node.get_path().replace("\\", "/")
    if "/lvgl/" not in path:
        return node
    if path.endswith(".S") and ("/helium/" in path or "/neon/" in path):
        return None
    return node


env.AddBuildMiddleware(_skip_lvgl_arm_asm)
