# Always use PlatformIO Core from .platformio\penv (Python 3.11).
# Do NOT use Python 3.14's "pio" — it breaks penv recreation (uv exit 106).
$Pio = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"
if (-not (Test-Path $Pio)) {
    Write-Error "PlatformIO penv not found: $Pio"
    exit 1
}
& $Pio @args
