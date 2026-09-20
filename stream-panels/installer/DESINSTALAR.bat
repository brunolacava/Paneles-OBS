@echo off
tasklist /FI "IMAGENAME eq obs64.exe" 2>nul | find /I "obs64.exe" >nul
if not errorlevel 1 (
  echo OBS esta abierto. Cierralo y vuelve a ejecutar este archivo.
  pause
  exit /b 1
)
rmdir /S /Q "%ProgramData%\obs-studio\plugins\stream-panels"
echo Stream Panels desinstalado.
pause
