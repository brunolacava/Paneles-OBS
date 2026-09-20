# Standalone test harness: builds the OBS-independent core (model, rendering,
# templates, UI) against plain Qt6 - no libobs required.
#
#   cmake -S tests -B build-tests -G Ninja
#   cmake --build build-tests
#   QT_QPA_PLATFORM=offscreen ./build-tests/render-test  <output dir>
#   QT_QPA_PLATFORM=offscreen ./build-tests/ui-shot      <output dir>
cmake_minimum_required(VERSION 3.20)
project(stream-panels-tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 COMPONENTS Svg)

include("${CMAKE_CURRENT_SOURCE_DIR}/../src/sources.cmake")

option(SP_TEST_WITH_UI "Also build the dock UI (needed by ui-shot)" ON)
if(SP_TEST_WITH_UI)
  set(SP_TEST_UI_SOURCES ${STREAMPANELS_UI_SOURCES})
endif()

add_library(sp-core STATIC ${STREAMPANELS_CORE_SOURCES} ${SP_TEST_UI_SOURCES})
target_include_directories(sp-core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_link_libraries(sp-core PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets)
if(TARGET Qt6::Svg)
  target_link_libraries(sp-core PUBLIC Qt6::Svg)
  target_compile_definitions(sp-core PUBLIC SP_HAVE_QTSVG=1)
endif()
if(NOT MSVC)
  target_compile_options(sp-core PRIVATE -Wall -Wextra -Wno-deprecated-enum-enum-conversion)
endif()
target_compile_definitions(sp-core PUBLIC SP_LOCALE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data/locale")

add_executable(render-test render-test.cpp test-support.hpp)
target_link_libraries(render-test PRIVATE sp-core)

add_executable(ui-shot ui-shot.cpp test-support.hpp)
target_link_libraries(ui-shot PRIVATE sp-core)
