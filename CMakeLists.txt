cmake_minimum_required(VERSION 3.28...3.30)

# Reads buildspec.json (name, version, author...) and downloads/locates the OBS
# sources + prebuilt obs-deps + prebuilt Qt6 (Windows / macOS).
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

# Stream Panels needs both: the frontend API (dock, scene access) and Qt (UI + rendering).
if(NOT ENABLE_FRONTEND_API OR NOT ENABLE_QT)
  message(FATAL_ERROR "Stream Panels requires ENABLE_FRONTEND_API=ON and ENABLE_QT=ON")
endif()

find_package(obs-frontend-api REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

# Qt6::Svg is shipped with OBS (Qt6Svg.dll) and enables crisp SVG logos.
# It is optional at build time: without it PNG/JPG logos still work.
find_package(Qt6 COMPONENTS Svg)
if(TARGET Qt6::Svg)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Svg)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE SP_HAVE_QTSVG=1)
else()
  message(WARNING "Qt6::Svg not found: SVG logos will be disabled")
endif()

target_compile_options(
  ${CMAKE_PROJECT_NAME}
  PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)

# Shared, OBS-independent code lives in src/ and is reused by tests/ (see tests/CMakeLists.txt).
include("${CMAKE_CURRENT_SOURCE_DIR}/src/sources.cmake")
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${STREAMPANELS_CORE_SOURCES} ${STREAMPANELS_UI_SOURCES} ${STREAMPANELS_OBS_SOURCES})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
