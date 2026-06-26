Import("env")

# Priority: app_name -> custom_prog_name -> default
cfg = env.GetProjectConfig()
section = "env:" + env["PIOENV"]

app_name = cfg.get(section, "app_name", None)
custom_name = cfg.get(section, "custom_prog_name", None)

project_name = app_name or custom_name or "RAK_os_app"
env.Replace(PROGNAME=project_name)
