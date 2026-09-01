cmake_minimum_required(VERSION 3.21)
project(omapdf VERSION 0.2.1 LANGUAGES CXX)

configure_file(
  ${CMAKE_SOURCE_DIR}/src/version.h.in
  ${CMAKE_BINARY_DIR}/generated/version.h
  @ONLY
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(OMAPDF_ASAN "Enable AddressSanitizer and UBSan in Debug" ON)
option(OMAPDF_CLANG_TIDY "Run clang-tidy while compiling" OFF)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Pdf Widgets PrintSupport Concurrent Test Qml)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBQPDF REQUIRED libqpdf)

qt_standard_project_setup(REQUIRES 6.8)

if(OMAPDF_CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY
    "${CMAKE_SOURCE_DIR}/scripts/clang-tidy-omapdf;--quiet;-p=${CMAKE_BINARY_DIR}")
endif()

add_library(omapdf_core STATIC
  src/app/ThemeBridge.cpp
  src/app/SessionStore.cpp
  src/app/AppController.cpp
  src/app/TabModel.cpp
  src/app/LinkGuard.cpp
  src/app/StructureEngine.cpp
  src/app/LocaleTranslator.cpp
)
target_include_directories(omapdf_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_BINARY_DIR}/generated
)
target_link_libraries(omapdf_core PUBLIC
  Qt6::Core
  Qt6::Gui
  Qt6::Quick
  Qt6::Pdf
  Qt6::Widgets
  Qt6::PrintSupport
  Qt6::Concurrent
  ${LIBQPDF_LINK_LIBRARIES}
)
target_include_directories(omapdf_core SYSTEM PUBLIC ${LIBQPDF_INCLUDE_DIRS})
target_compile_options(omapdf_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
qt_add_executable(omapdf src/main.cpp translations/i18n.qrc)
qt_add_qml_module(omapdf
  URI Omapdf
  VERSION 1.0
  SOURCES
    src/app/AnnotStore.h
    src/app/AnnotStore.cpp
    src/app/PageWarmup.h
    src/app/PageWarmup.cpp
    src/app/PageTileLayer.h
    src/app/PageTileLayer.cpp
    src/app/PageTileHub.h
    src/app/PageTileHub.cpp
  QML_FILES
    qml/Main.qml
    qml/chrome/TabStrip.qml
    qml/chrome/StatusBar.qml
    qml/chrome/SearchBar.qml
    qml/chrome/ThemedButton.qml
    qml/chrome/ThemedPopup.qml
    qml/chrome/ThemedToolButton.qml
    qml/chrome/RailButton.qml
    qml/chrome/ToolRail.qml
    qml/chrome/PasswordDialog.qml
    qml/chrome/LinkConfirmDialog.qml
    qml/chrome/DirtyCloseDialog.qml
    qml/chrome/DeletePageDialog.qml
    qml/viewer/PdfPane.qml
    qml/viewer/WelcomePage.qml
    qml/viewer/OutlinePane.qml
    qml/viewer/ThumbnailPane.qml
    qml/viewer/OmapdfMultiPageView.qml
  RESOURCES
    assets/omapdf-logo.svg
)

target_include_directories(omapdf PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/app
)
target_link_libraries(omapdf PRIVATE omapdf_core Qt6::QuickControls2 Qt6::Qml)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(omapdf_core PRIVATE -O3)
  target_compile_options(omapdf PRIVATE -O3)
  target_compile_definitions(omapdf_core PRIVATE NDEBUG)
  target_compile_definitions(omapdf PRIVATE NDEBUG)
elseif(OMAPDF_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  foreach(t omapdf_core omapdf)
    target_compile_options(${t} PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${t} PUBLIC -fsanitize=address,undefined)
  endforeach()
endif()

enable_testing()
qt_add_executable(omapdf_theme_test tests/test_theme_bridge.cpp)
target_link_libraries(omapdf_theme_test PRIVATE omapdf_core Qt6::Test)
add_test(NAME theme_bridge COMMAND omapdf_theme_test)

qt_add_executable(omapdf_annot_test
  tests/test_annot_store.cpp
  src/app/AnnotStore.cpp
)
target_include_directories(omapdf_annot_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/app
)
target_link_libraries(omapdf_annot_test PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Test)
target_compile_options(omapdf_annot_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME annot_store COMMAND omapdf_annot_test)

qt_add_executable(omapdf_structure_test tests/test_structure_engine.cpp)
target_link_libraries(omapdf_structure_test PRIVATE omapdf_core Qt6::Test)
add_test(NAME structure_engine COMMAND omapdf_structure_test)

qt_add_executable(omapdf_flatten_test tests/test_flatten_export.cpp)
target_link_libraries(omapdf_flatten_test PRIVATE omapdf_core Qt6::Test)
add_test(NAME flatten_export COMMAND omapdf_flatten_test)

qt_add_executable(omapdf_locale_test tests/test_locale_translator.cpp)
target_link_libraries(omapdf_locale_test PRIVATE omapdf_core Qt6::Test)
target_compile_definitions(omapdf_locale_test PRIVATE OMAPDF_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME locale_translator COMMAND omapdf_locale_test)


qt_add_executable(omapdf_reading_gate_test tests/test_reading_gate.cpp)
target_link_libraries(omapdf_reading_gate_test PRIVATE Qt6::Test Qt6::Pdf Qt6::Gui)
target_compile_options(omapdf_reading_gate_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME reading_gate COMMAND omapdf_reading_gate_test)


qt_add_executable(omapdf_app_controller_test tests/test_app_controller.cpp)
target_link_libraries(omapdf_app_controller_test PRIVATE omapdf_core Qt6::Test Qt6::Gui Qt6::Pdf)
target_compile_options(omapdf_app_controller_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME app_controller COMMAND omapdf_app_controller_test)


qt_add_executable(omapdf_session_test tests/test_session_store.cpp)
target_link_libraries(omapdf_session_test PRIVATE omapdf_core Qt6::Test)
add_test(NAME session_store COMMAND omapdf_session_test)

qt_add_executable(omapdf_page_tile_geometry_test tests/test_page_tile_geometry.cpp)
target_link_libraries(omapdf_page_tile_geometry_test PRIVATE Qt6::Test Qt6::Gui)
target_include_directories(omapdf_page_tile_geometry_test PRIVATE src)
add_test(NAME page_tile_geometry COMMAND omapdf_page_tile_geometry_test)

qt_add_executable(omapdf_viewer_page_image_test tests/test_viewer_page_image.cpp)
target_link_libraries(omapdf_viewer_page_image_test PRIVATE Qt6::Test)
target_compile_definitions(omapdf_viewer_page_image_test PRIVATE OMAPDF_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME viewer_page_image COMMAND omapdf_viewer_page_image_test)




install(TARGETS omapdf RUNTIME DESTINATION bin)
install(FILES packaging/omapdf.desktop DESTINATION share/applications)
install(FILES packaging/omapdf.svg DESTINATION share/icons/hicolor/scalable/apps)
