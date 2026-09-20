# Stream Panels

Plugin **nativo** de OBS Studio para diseñar y controlar paneles de texto profesionales
(lower thirds) sin salir de OBS. Sin Browser Source, sin HTML, sin apps externas.

- **Fuente de OBS** `Stream Panel`: se añade desde *Fuentes > +*, se ve en Preview, Program,
  grabación y stream, y se mueve/escala/filtra con las herramientas normales de OBS.
- **Dock "Stream Panels"**: biblioteca de paneles, preview en vivo, 4 diseños, logo, colores,
  tipografías, ancho/alto automático o fijo.
- Plugin ID `com.brunolacava.streampanels` · versión `1.0.0` · autor Bruno Lacava · GPL-2.0-or-later.

Compilar: ver **[BUILD.md](BUILD.md)**. Capturas de los diseños y del dock: `docs/preview/`.

## Arquitectura

```
src/
  plugin-main.cpp            OBS: módulo, dock, ciclo de vida, wiring
  obs/                       OBS: única capa que incluye libobs / obs-frontend-api
    panel-source.*           fuente "Stream Panel" (textura + video_render + propiedades)
    obs-scene-bridge.*       "añadir a escena", contar/desvincular fuentes
  model/                     DATOS (solo Qt Core)
    panel-model.*            PanelModel: valor puro + JSON
    panel-manager.*          PanelManager: biblioteca + persistencia atómica (panels.json)
    logo-store.*             importa logos a la config del plugin, rutas relativas/absolutas
  render/                    RENDER (Qt Gui)
    panel-renderer.*         PanelModel -> QImage premultiplicada (único camino de pixeles)
    text-measurer.*          medición real de texto (QFontMetricsF), nunca por nº de caracteres
    logo-source.*            decodifica y cachea PNG/JPG/SVG
  templates/                 DISEÑOS
    panel-template.*         clase base: sizing (ancho/alto) + helpers
    template-{classic,minimal,tag,modern}.*
    template-registry.*      lista de diseños; el único sitio a tocar para añadir uno
  ui/                        UI (solo Qt Widgets, no conoce OBS)
    panel-editor.*           el dock; borradores por panel
    scene-bridge.hpp         interfaz hacia OBS (implementada en obs/)
    preview-widget, template-picker, color-button, theme
  common/platform.*          hooks de log y textos (obs_log / obs_module_text)
data/locale/                 en-US.ini, es-ES.ini
tests/                       harness Qt independiente de OBS
```

Solo `obs/` y `plugin-main.cpp` dependen de libobs. Todo lo demás compila y se prueba con Qt
solo, lo que permite verificar renderizado y UI sin abrir OBS.

### Cómo se renderiza (y por qué)

```
 PanelModel ──► PanelRenderer ──► QImage (RGBA8888 premultiplicada, 2x por defecto)
   (hilo UI)     Template.layout()          │
                 Template.paint()           ▼
                              gs_texture_create()  ──►  video_render(): gs_draw_sprite()
                                   (hilo gráfico)        (misma receta que la fuente de imagen de OBS)
```

- El panel es una **textura de OBS** dibujada con el sistema gráfico de OBS: no hay ventana Qt capturada.
- El texto **nunca** se rasteriza en el hilo gráfico. En OBS, `update()` de una fuente de vídeo se
  ejecuta diferido en el hilo gráfico, así que el render se encola al hilo de UI de Qt
  (`renderJob`) y el resultado vuelve por un búfer protegido con mutex. Un contador de
  generación descarta resultados obsoletos.
- La preview del dock usa **el mismo `PanelRenderer`**: lo que ves es lo que sale al aire.
- El render es supersampleado (`Resolución de render`, 2x por defecto) y se dibuja al tamaño
  lógico, así que se ve nítido aunque escales la fuente en OBS.

### Auto-ajuste de tamaño

`PanelTemplate::layout()` resuelve el tamaño en dos pasos:

1. **Ancho**: modo *Ajustar al texto* → `naturalWidth()` de la plantilla (ancho real medido con
   `QFontMetricsF` del título/texto + logo + paddings + separaciones) recortado a
   `[mínimo, máximo]`. Modo *Personalizado* → exactamente el ancho indicado.
2. **Altura**: `layoutAtWidth()` envuelve el texto por palabras al ancho resultante y calcula la
   altura (automática) o usa la altura fija (el texto se recorta, nunca se sale del panel).

Si el texto supera el ancho máximo, **se parte en varias líneas** en vez de cortarse.
Los elementos de tamaño fijo (bloque del logo, etc.) se derivan de la altura de línea, no de la
altura envuelta, para evitar dependencias circulares.

### Sistema de templates

Un diseño implementa `PanelTemplate`: `defaults()` (colores/fuentes/paddings), `naturalWidth()`,
`layoutAtWidth()` y `paint()`. Ejemplo mínimo de alta:

```cpp
// template-registry.cpp
registerTemplate(std::make_unique<MyTemplate>());
```

y añadir dos líneas en `sources.cmake` y `Template.<id>` en los `.ini`. El editor, el selector
visual (miniaturas renderizadas por el propio renderer), la preview y la fuente lo reconocen solos.

### Persistencia

| Qué | Dónde |
|---|---|
| Biblioteca de paneles | `%APPDATA%\obs-studio\plugin_config\stream-panels\panels.json` (escritura atómica con `QSaveFile`; un JSON corrupto se aparta como `.corrupt-<fecha>`) |
| Logos | Se **importan** a `…\stream-panels\logos\<sha1>.<ext>`; el panel guarda la ruta *relativa* → sobrevive a mover/borrar el original. Si no se puede copiar, guarda la ruta absoluta |
| Cada fuente en la escena | `panel_id` + `snapshot` (copia JSON del panel) en los ajustes de la fuente → se guarda con la colección de escenas y sigue funcionando aunque la biblioteca no tenga ese panel (se re-importa) |

### Flujo de edición

Cada panel tiene un **borrador**. Los controles modifican el borrador y la preview lo refleja al
instante; nada llega a las fuentes en pantalla hasta **ACTUALIZAR** (o con *Actualizar en vivo*).
**GUARDAR** además fuerza escritura a disco (también se guarda solo al crear/borrar/duplicar y al
cerrar OBS). **AÑADIR A ESCENA** aplica el borrador y crea la fuente (en Modo Estudio, en la
escena de preview). **ELIMINAR** avisa cuántas fuentes lo usan; estas conservan su aspecto como
copias independientes.

## Decisiones y límites conocidos

- **Animaciones**: no están en el MVP, pero `PanelModel` ya persiste `animationIn/Out`
  (tipo + duración) para no romper archivos cuando se implementen.
- **Ancho mínimo/máximo** aplican solo en *Ajustar al texto*; en *Personalizado* manda el valor exacto.
- **Logos en otra máquina**: la colección de escenas lleva el diseño del panel, pero los archivos
  de logo viven en la carpeta de configuración; al mover la colección hay que copiar también `logos\`.
- **SVG** requiere `Qt6::Svg` (viene con OBS); si falta en la compilación, se desactiva y solo se aceptan PNG/JPG/BMP/WebP.
- Compatibilidad hacia adelante: solo se usa API pública y estable (`obs_register_source`,
  `obs_frontend_add_dock_by_id`, `obs_module_config_path`, `gs_*`). macOS/Linux comparten el mismo
  código; falta probarlos (el objetivo primero es Windows x64).
