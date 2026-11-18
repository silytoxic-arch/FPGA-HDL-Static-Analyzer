cmake_minimum_required(VERSION 3.24)
project(FPGAAnalyzer VERSION 1.4.208 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(FPGA_ANALYZER_BUILD_GUI "Build the Qt desktop application" ON)
include(CTest)
include(GNUInstallDirs)

add_library(fpga_build_options INTERFACE)
target_compile_definitions(fpga_build_options INTERFACE
    $<$<PLATFORM_ID:Windows>:NOMINMAX>)
target_compile_options(fpga_build_options INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive- /utf-8>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>)

add_library(fpga_core STATIC
    src/analysis/analyzer.cpp
    src/analysis/baseline.cpp
    src/analysis/line_statistics.cpp
    src/analysis/rule_catalog.cpp
    src/analysis/rule_localization.cpp
    src/analysis/standard_profiles.cpp
    src/analysis/standard_evaluator.cpp
    src/core/application_info.cpp
    src/core/project.cpp
    src/database/design_database.cpp
    src/parser/design_parser.cpp
    src/parser/preprocessor.cpp
    src/parser/semantic_analyzer.cpp
    src/parser/timing_analyzer.cpp
    src/project/config_file.cpp
    src/project/file_list.cpp
    src/report/report_writer.cpp)
target_include_directories(fpga_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(fpga_core PRIVATE fpga_build_options)

add_executable(fpga_analyzer_cli src/cli/main.cpp src/cli/command_line.cpp)
target_link_libraries(fpga_analyzer_cli PRIVATE fpga_core fpga_build_options)

if(FPGA_ANALYZER_BUILD_GUI)
    find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Concurrent)
    add_library(fpga_gui STATIC src/gui/main_window.cpp src/gui/main_window.h)
    target_link_libraries(fpga_gui PUBLIC fpga_core Qt6::Widgets Qt6::Concurrent PRIVATE fpga_build_options)
    qt_add_resources(FPGA_BRANDING_RESOURCES src/gui/branding.qrc)
    # One owning target for generated resource source. Sharing its filename
    # across root/tests directories loses the custom-command dependency in
    # fresh parallel Makefile builds; share compiled objects instead.
    add_library(fpga_branding OBJECT ${FPGA_BRANDING_RESOURCES})
    target_link_libraries(fpga_branding PRIVATE Qt6::Core)
    add_executable(fpga_analyzer WIN32 MACOSX_BUNDLE src/gui/main.cpp $<TARGET_OBJECTS:fpga_branding>)
    target_link_libraries(fpga_analyzer PRIVATE fpga_gui fpga_build_options)
    set_target_properties(fpga_analyzer PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "FPGA HDL Static Analyzer"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.fpgaanalyzer.desktop"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "")
    if(APPLE)
        set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/branding/hdl-analyzer.icns")
        target_sources(fpga_analyzer PRIVATE "${APP_ICON}")
        set_source_files_properties("${APP_ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        set_property(TARGET fpga_analyzer PROPERTY MACOSX_BUNDLE_ICON_FILE hdl-analyzer.icns)
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt
            HINTS "${Qt6_DIR}/../../../bin"
            REQUIRED)
        find_program(QMAKE_EXECUTABLE qmake
            HINTS "${Qt6_DIR}/../../../bin"
            REQUIRED)
        find_program(INSTALL_NAME_TOOL_EXECUTABLE install_name_tool REQUIRED)
        find_program(CODESIGN_EXECUTABLE codesign REQUIRED)
        execute_process(
            COMMAND "${QMAKE_EXECUTABLE}" -query QT_INSTALL_PLUGINS
            OUTPUT_VARIABLE QT_INSTALL_PLUGINS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY)
        add_custom_command(TARGET fpga_analyzer POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E remove_directory
                    "$<TARGET_BUNDLE_DIR:fpga_analyzer>/Contents/PlugIns"
            COMMAND "${CMAKE_COMMAND}" -E remove_directory
                    "$<TARGET_BUNDLE_DIR:fpga_analyzer>/Contents/Frameworks"
            COMMAND "${MACDEPLOYQT_EXECUTABLE}"
                    "$<TARGET_BUNDLE_DIR:fpga_analyzer>"
                    -always-overwrite
                    -no-plugins
                    -no-codesign
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                    "$<TARGET_BUNDLE_DIR:fpga_analyzer>/Contents/PlugIns/platforms"
                    "$<TARGET_BUNDLE_DIR:fpga_analyzer>/Contents/PlugIns/styles"
            COMMAND "${CMAKE_COMMAND}" -E copy
                    "${QT_INSTALL_PLUGINS}/platforms/libqcocoa.dylib"
                    "$<TARGET_BUNDLE_DIR:fpga_analyzer>/Contents/PlugIns/platforms/libqcocoa.dylib"
            COMMAND "${CMAKE_COMMAND}" -E copy
                    "${QT_INSTALL_PLUGINS}/styles/libqmacstyle.dylib"
                    "$<TARGET_BUNDLE_DIR:fpga_analyzer>/Contents/PlugIns/styles/libqmacstyle.dylib"
            COMMAND "${INSTALL_NAME_TOOL_EXECUTABLE}" -delete_rpath
                    "@loader_path/../../../../lib"
                    "$<TARGET_BUNDLE_DIR:fpga_analyzer>/Contents/PlugIns/platforms/libqcocoa.dylib"
            COMMAND "${INSTALL_NAME_TOOL_EXECUTABLE}" -add_rpath
                    "@loader_path/../../Frameworks"
                    "$<TARGET_BUNDLE_DIR:fpga_analyzer>/Contents/PlugIns/platforms/libqcocoa.dylib"
            COMMAND "${INSTALL_NAME_TOOL_EXECUTABLE}" -delete_rpath
                    "@loader_path/../../../../lib"
                    "$<TARGET_BUNDLE_DIR:fpga_analyzer>/Contents/PlugIns/styles/libqmacstyle.dylib"
            COMMAND "${INSTALL_NAME_TOOL_EXECUTABLE}" -add_rpath
                    "@loader_path/../../Frameworks"
                    "$<TARGET_BUNDLE_DIR:fpga_analyzer>/Contents/PlugIns/styles/libqmacstyle.dylib"
            COMMAND "${CMAKE_COMMAND}" -E copy
                    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/qt.conf"
                    "$<TARGET_BUNDLE_DIR:fpga_analyzer>/Contents/Resources/qt.conf"
            COMMAND "${CODESIGN_EXECUTABLE}" --force --deep --sign -
                    "$<TARGET_BUNDLE_DIR:fpga_analyzer>"
            COMMENT "Deploying Qt frameworks and plugins into fpga_analyzer.app"
            VERBATIM)
    endif()
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(TARGETS fpga_analyzer_cli
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
if(FPGA_ANALYZER_BUILD_GUI)
    install(TARGETS fpga_analyzer
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        BUNDLE DESTINATION ".")
    if(WIN32)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt
            HINTS "${Qt6_DIR}/../../../bin"
            REQUIRED)
        install(CODE
            "execute_process(COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" --release --dir \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\" \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/fpga_analyzer.exe\" COMMAND_ERROR_IS_FATAL ANY)")
    endif()
endif()
install(FILES README.md
    DESTINATION "${CMAKE_INSTALL_DOCDIR}")
install(DIRECTORY docs/
    DESTINATION "${CMAKE_INSTALL_DOCDIR}")
install(DIRECTORY examples/
    DESTINATION "${CMAKE_INSTALL_DATADIR}/fpga-analyzer/examples")

set(CPACK_PACKAGE_NAME "FPGA-HDL-Static-Analyzer")
set(CPACK_PACKAGE_VENDOR "FPGA HDL Static Analyzer Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Static analysis, lint, RTL risk, and design-structure analysis for FPGA HDL")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_GENERATOR "ZIP")
include(CPack)
