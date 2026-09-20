# Source lists shared between the plugin (CMakeLists.txt) and the standalone
# test harness (tests/CMakeLists.txt).
#
#   CORE   -> Qt only. No dependency on libobs. Model, rendering, templates.
#   UI     -> Qt Widgets only (dock editor). Talks to OBS through ui/scene-bridge.hpp.
#   OBS    -> everything that talks to libobs / obs-frontend-api.

set(_sp "${CMAKE_CURRENT_LIST_DIR}")

set(
  STREAMPANELS_CORE_SOURCES
  ${_sp}/common/platform.hpp
  ${_sp}/common/platform.cpp
  ${_sp}/model/panel-model.hpp
  ${_sp}/model/panel-model.cpp
  ${_sp}/model/logo-store.hpp
  ${_sp}/model/logo-store.cpp
  ${_sp}/model/panel-manager.hpp
  ${_sp}/model/panel-manager.cpp
  ${_sp}/render/text-measurer.hpp
  ${_sp}/render/text-measurer.cpp
  ${_sp}/render/logo-source.hpp
  ${_sp}/render/logo-source.cpp
  ${_sp}/render/panel-renderer.hpp
  ${_sp}/render/panel-renderer.cpp
  ${_sp}/templates/panel-template.hpp
  ${_sp}/templates/panel-template.cpp
  ${_sp}/templates/template-registry.hpp
  ${_sp}/templates/template-registry.cpp
  ${_sp}/templates/template-classic.hpp
  ${_sp}/templates/template-classic.cpp
  ${_sp}/templates/template-minimal.hpp
  ${_sp}/templates/template-minimal.cpp
  ${_sp}/templates/template-tag.hpp
  ${_sp}/templates/template-tag.cpp
  ${_sp}/templates/template-modern.hpp
  ${_sp}/templates/template-modern.cpp
)

set(
  STREAMPANELS_UI_SOURCES
  ${_sp}/ui/theme.hpp
  ${_sp}/ui/theme.cpp
  ${_sp}/ui/color-button.hpp
  ${_sp}/ui/color-button.cpp
  ${_sp}/ui/preview-widget.hpp
  ${_sp}/ui/preview-widget.cpp
  ${_sp}/ui/template-picker.hpp
  ${_sp}/ui/template-picker.cpp
  ${_sp}/ui/scene-bridge.hpp
  ${_sp}/ui/panel-editor.hpp
  ${_sp}/ui/panel-editor.cpp
)

set(
  STREAMPANELS_OBS_SOURCES
  ${_sp}/plugin-main.cpp
  ${_sp}/obs/panel-source.hpp
  ${_sp}/obs/panel-source.cpp
  ${_sp}/obs/obs-scene-bridge.hpp
  ${_sp}/obs/obs-scene-bridge.cpp
)

unset(_sp)
