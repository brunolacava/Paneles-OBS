@echo off
setlocal
cd /d "%~dp0"
echo ============================================
echo   Stream Panels - instalador para OBS 32
echo ============================================
echo.

if not exist "%~dp0stream-panels\bin\64bit\stream-panels.dll" (
  echo [ERROR] Falta stream-panels\bin\64bit\stream-panels.dll. Descomprime el ZIP completo primero.
  goto :fin
)

tasklist /FI "IMAGENAME eq obs64.exe" 2>nul | find /I "obs64.exe" >nul
if not errorlevel 1 (
  echo [ERROR] OBS esta abierto. Cierralo y vuelve a ejecutar este archivo.
  goto :fin
)

set "DEST=%ProgramData%\obs-studio\plugins\stream-panels"
xcopy "%~dp0stream-panels" "%DEST%" /E /I /Y /Q >nul
if errorlevel 1 (
  echo [ERROR] No se pudo copiar a %DEST%
  echo         Prueba con clic derecho ^> "Ejecutar como administrador".
  goto :fin
)

echo Instalado en: %DEST%
echo.
echo LISTO. Abre OBS: menu Docks ^> Stream Panels
:fin
echo.
pause
endlocal
