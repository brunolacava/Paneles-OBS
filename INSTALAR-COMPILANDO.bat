@echo off
setlocal
cd /d "%~dp0"
echo ============================================
echo   Stream Panels - compilar e instalar en OBS
echo ============================================
echo.

where cmake >nul 2>nul
if errorlevel 1 (
  echo [ERROR] No se encuentra CMake. Instala CMake 3.28 o superior desde https://cmake.org/download/
  echo         y marca "Add CMake to the system PATH". Luego vuelve a ejecutar este archivo.
  goto :fin
)

tasklist /FI "IMAGENAME eq obs64.exe" 2>nul | find /I "obs64.exe" >nul
if not errorlevel 1 (
  echo [ERROR] OBS esta abierto. Cierralo y vuelve a ejecutar este archivo.
  goto :fin
)

echo [1/3] Configurando (la primera vez descarga ~350 MB, puede tardar)...
cmake --preset windows-x64
if errorlevel 1 (
  echo.
  echo [ERROR] Fallo la configuracion. Necesitas Visual Studio 2022 con "Desarrollo para el escritorio con C++"
  echo         y el Windows SDK 10.0.22621.
  goto :fin
)

echo.
echo [2/3] Compilando...
cmake --build --preset windows-x64 --config Release
if errorlevel 1 (
  echo.
  echo [ERROR] Fallo la compilacion. Copia el mensaje de error de arriba y enviamelo.
  goto :fin
)

echo.
echo [3/3] Instalando en OBS...
cmake --install build_x64 --config Release
if errorlevel 1 (
  echo.
  echo [ERROR] No se pudo instalar. Prueba con clic derecho ^> "Ejecutar como administrador".
  goto :fin
)

echo.
echo ============================================
echo   LISTO. Abre OBS: menu Docks ^> Stream Panels
echo ============================================
:fin
echo.
pause
endlocal
