Import("env")

# Auto-detect serial port for upload and monitor if not explicitly set.
try:
    opt = env.GetProjectOption("upload_port", None)
    if not opt or opt == "auto":
        port = env.AutodetectUploadPort()
        if port:
            env.Replace(UPLOAD_PORT=port)
            monitor = env.GetProjectOption("monitor_port", None)
            if not monitor or monitor == "auto":
                env.Replace(MONITOR_PORT=port)
        else:
            print("[auto_port] No serial device detected; keep UPLOAD_PORT = auto")
except Exception as e:
    print("[auto_port] Warning:", e)
