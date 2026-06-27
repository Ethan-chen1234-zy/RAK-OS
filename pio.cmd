@echo off
REM RAKOS: always use PlatformIO Core from ~/.platformio/penv (Python 3.11).
REM Avoids system Python 3.14 "pio" breaking penv (uv exit 106).
set "PIO=%USERPROFILE%\.platformio\penv\Scripts\pio.exe"
if not exist "%PIO%" (
  echo [RAKOS] PlatformIO penv not found: %PIO%
  echo See dist\BUILD_NOTES.txt to recreate penv.
  exit /b 1
)
"%PIO%" %*
