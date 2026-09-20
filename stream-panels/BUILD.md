# Compilar Stream Panels en Windows (Visual Studio 2022)

Plugin nativo para **OBS Studio 32.2.2** (Windows x64). Usa la estructura oficial
de [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate):
`buildspec.json` + `CMakePresets.json` + carpeta `cmake/`.

> Estado de verificación: el núcleo (modelo, renderizado, templates, UI) se compiló y probó
> en Linux con Qt 6 (`tests/`), y las tres unidades que hablan con OBS
> (`panel-source.cpp`, `obs-scene-bridge.cpp`, `plugin-main.cpp`) se compilaron contra las
> cabeceras reales de OBS 32.2.2. **No se ha ejecutado dentro de OBS ni compilado con MSVC**:
> ese es el primer paso que harás tú con esta guía.

---

## 1. Dependencias

| Necesitas | Detalle |
|---|---|
| Windows 10/11 x64 | |
| **Visual Studio 2022** (17.9 o superior) | Carga de trabajo **"Desarrollo para el escritorio con C++"** |
| **Windows SDK 10.0.22621** (Windows 11 SDK) | Se instala desde el instalador de VS (componentes individuales). El preset lo exige; ver nota abajo si tienes otro |
| **CMake ≥ 3.28** | Viene con VS 2022 17.9+, o instálalo desde <https://cmake.org/download/> y márcalo en el PATH |
| Conexión a Internet (solo la 1.ª vez) | Descarga ~350 MB: dependencias precompiladas de OBS (67 MB), Qt 6.11 de OBS (261 MB) y el código fuente de OBS 32.2.2 (19 MB) |
| **OBS Studio 32.2.2** instalado | Para probar el plugin |

No hace falta instalar Qt ni compilar OBS: se usan **exactamente** los binarios de Qt y las
librerías que OBS 32.2.2 usa en su propia compilación.

> **Otro Windows SDK:** edita `CMakePresets.json`, preset `windows-x64`, campo
> `"architecture": "x64,version=10.0.22621"` y pon la versión que tengas (mínimo 10.0.20348).

## 2. Obtener el código fuente de OBS

**Es automático.** `buildspec.json` fija la versión `32.2.2` y el hash SHA-256 de cada descarga;
al configurar, CMake descarga y descomprime todo en `.deps\`:

```
.deps\obs-studio-32.2.2\     <- código fuente de OBS (libobs, obs-frontend-api)
.deps\obs-deps-2026-07-15-x64\
.deps\obs-deps-qt6-2026-07-15-x64\
```

No tienes que clonar nada. (Si algún día GitHub regenera el zip y falla la comprobación de
hash, recalcula con `Get-FileHash 32.2.2.zip -Algorithm SHA256` y actualiza
`dependencies.obs-studio.hashes.windows-x64` en `buildspec.json`.)

## 3. Configurar CMake

Abre **PowerShell** (no hace falta el prompt de VS) en la carpeta del proyecto:

```powershell
cd stream-panels
cmake --preset windows-x64
```

Genera la solución `build_x64\stream-panels.sln`. La 1.ª vez tarda unos minutos por las descargas.

Los warnings no se tratan como errores (`CMAKE_COMPILE_WARNING_AS_ERROR=OFF` en el preset), para
que un aviso nuevo de un compilador futuro no te bloquee. Los presets `*-ci-*` sí los activan.

## 4. Compilar

Desde línea de comandos:

```powershell
# Release (para distribuir)
cmake --build --preset windows-x64 --config Release

# RelWithDebInfo (recomendado para depurar: optimizado con símbolos .pdb)
cmake --build --preset windows-x64 --config RelWithDebInfo

# Debug
cmake --build --preset windows-x64 --config Debug
```

O desde Visual Studio: abre `build_x64\stream-panels.sln`, elige la configuración en la barra
superior (`Release`, `RelWithDebInfo`, `Debug`) y **Compilar > Compilar solución** (Ctrl+Shift+B).

> **Sobre Debug:** los paquetes precompilados de OBS (Qt, libobs) son de Release. Un plugin en
> configuración `Debug` puro usa el runtime de depuración de MSVC y puede no enlazar o no cargar
> correctamente. Para depurar con puntos de interrupción usa **RelWithDebInfo**, que es lo que
> usa el propio equipo de OBS. (La configuración `Debug` está disponible en el proyecto, pero
> no la he podido verificar contra esos paquetes.)

## 5. Dónde queda el plugin generado

Tras compilar, un paso post-build copia todo a `build_x64\rundir\<Config>\`:

```
build_x64\rundir\Release\stream-panels.dll
build_x64\rundir\Release\stream-panels.pdb
build_x64\rundir\Release\stream-panels\locale\en-US.ini
build_x64\rundir\Release\stream-panels\locale\es-ES.ini
```

(La DLL "cruda" también está en `build_x64\Release\`.)

## 6. Instalar

### Opción A (recomendada): `cmake --install`, ruta oficial de plugins de OBS

```powershell
# Si falla por permisos de C:\ProgramData, repite desde un PowerShell como administrador
cmake --install build_x64 --config Release
```

Instala en `%ALLUSERSPROFILE%\obs-studio\plugins\` (normalmente `C:\ProgramData\obs-studio\plugins\`):

```
C:\ProgramData\obs-studio\plugins\stream-panels\
    bin\64bit\stream-panels.dll
    bin\64bit\stream-panels.pdb
    data\locale\en-US.ini
    data\locale\es-ES.ini
```

OBS 28+ escanea esa carpeta automáticamente. Para otro destino:
`cmake --install build_x64 --config Release --prefix "D:\mi-carpeta"`.

### Opción B: estructura clásica dentro de la carpeta de OBS

```
C:\Program Files\obs-studio\
    obs-plugins\64bit\stream-panels.dll
    data\obs-plugins\stream-panels\locale\*.ini
```

Con el script incluido (cierra OBS antes; PowerShell **como administrador**):

```powershell
.\scripts\install-legacy-windows.ps1 -Config Release
# o, si OBS está en otra ruta:
.\scripts\install-legacy-windows.ps1 -Config Release -ObsDir "D:\Apps\obs-studio"
```

Usa **una sola** de las dos opciones; si instalas en ambas, OBS cargará dos copias y avisará.

## 7. Verificar que OBS lo cargó

1. Abre OBS. Menú **Ayuda > Archivos de registro > Ver registro actual**.
2. Busca estas líneas:
   ```
   [stream-panels] plugin loaded successfully (version 1.0.0)
   ```
   y comprueba que **no** aparece `Failed to load 'stream-panels'` ni
   `Module '...stream-panels.dll' not loaded`.
3. **Docks (Paneles) > Stream Panels**: activa el dock (aparece flotante la 1.ª vez; puedes anclarlo).
4. En **Fuentes > + > Stream Panel** debe existir la fuente.
5. Prueba de humo: en el dock pulsa **AÑADIR A ESCENA**; el panel aparece abajo a la izquierda en la
   vista previa. Cambia el título y pulsa **ACTUALIZAR**: la fuente se actualiza sin recrearla.

### Si no carga

| Síntoma en el log | Causa probable |
|---|---|
| `LoadLibrary failed ... error 126/193` | Instalada en la carpeta equivocada, o DLL de 32 bits / falta un runtime de VC++ (`vc_redist.x64.exe`) |
| `Module ... is not compatible` / versión | Compilaste contra otra versión de OBS: revisa `buildspec.json` |
| Carga pero no aparece el dock | Activa **Docks > Stream Panels**; mira el log por `could not add the Stream Panels dock` |
| Textos de la interfaz como `Panels.New` | No se copiaron los `.ini` de `data\locale` |

Configuración y logos importados se guardan en
`%APPDATA%\obs-studio\plugin_config\stream-panels\` (`panels.json`, `logos\`).

## 8. Pruebas sin OBS (Linux/macOS/Windows con Qt 6)

El núcleo no depende de libobs, así que puedes probar renderizado y UI sin abrir OBS:

```bash
cmake -S tests -B build-tests -G Ninja
cmake --build build-tests
QT_QPA_PLATFORM=offscreen ./build-tests/render-test out/   # PNGs de todos los templates + comprobaciones
QT_QPA_PLATFORM=offscreen ./build-tests/ui-shot out/       # capturas del dock + flujo del editor
```
