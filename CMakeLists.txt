cmake_minimum_required(VERSION 3.21)

if(APPLE)
    # SerialTest intentionally targets Apple Silicon Macs only.
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Supported macOS architecture")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum supported macOS version")
endif()

project(SerialTest VERSION 0.3.5 LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src/ui")

find_package(
    Qt6 6.8 REQUIRED
    COMPONENTS
        Bluetooth
        Core
        Core5Compat
        Gui
        Network
        PrintSupport
        SerialPort
        Widgets
)

set(SERIALTEST_SOURCES
    src/adaptivestackedwidget.cpp
    src/appstyle.cpp
    src/asynccrc.cpp
    src/connection.cpp
    src/controlitem.cpp
    src/ctrltab.cpp
    src/datatab.cpp
    src/devicetab.cpp
    src/filetab.cpp
    src/filexceiver.cpp
    src/legenditemdialog.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/metadata.cpp
    src/mycustomplot.cpp
    src/mysettings.cpp
    src/plottab.cpp
    src/qcustomplot.cpp
    src/serialpinout.cpp
    src/sessiondata.cpp
    src/settingstab.cpp
    src/util.cpp
    src/adaptivestackedwidget.h
    src/appstyle.h
    src/asynccrc.h
    src/connection.h
    src/controlitem.h
    src/ctrltab.h
    src/datatab.h
    src/devicetab.h
    src/filetab.h
    src/filexceiver.h
    src/legenditemdialog.h
    src/mainwindow.h
    src/metadata.h
    src/mycustomplot.h
    src/mysettings.h
    src/plottab.h
    src/qcustomplot.h
    src/serialpinout.h
    src/sessiondata.h
    src/settingstab.h
    src/util.h
    src/ui/controlitem.ui
    src/ui/ctrltab.ui
    src/ui/datatab.ui
    src/ui/devicetab.ui
    src/ui/filetab.ui
    src/ui/legenditemdialog.ui
    src/ui/mainwindow.ui
    src/ui/plottab.ui
    src/ui/serialpinout.ui
    src/ui/settingstab.ui
    src/qdarkstyle/dark/darkstyle.qrc
    src/qdarkstyle/light/lightstyle.qrc
    src/theme/theme.qrc
)

if(WIN32)
    list(APPEND SERIALTEST_SOURCES
        src/windows.rc
        src/winrtbluetooth.cpp
        src/winrtbluetooth.h
    )
endif()

if(APPLE)
    list(APPEND SERIALTEST_SOURCES src/icon/icon.icns)
    set_source_files_properties(
        src/icon/icon.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
    )
endif()

qt_add_executable(SerialTest WIN32 MACOSX_BUNDLE ${SERIALTEST_SOURCES})

target_include_directories(SerialTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(
    SerialTest
    PRIVATE
        APP_VERSION="${PROJECT_VERSION}"
        QT_DEPRECATED_WARNINGS
)
target_link_libraries(
    SerialTest
    PRIVATE
        Qt6::Bluetooth
        Qt6::Core
        Qt6::Core5Compat
        Qt6::Gui
        Qt6::Network
        Qt6::PrintSupport
        Qt6::SerialPort
        Qt6::Widgets
)

if(MSVC)
    target_compile_definitions(
        SerialTest
        PRIVATE _SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS
    )
    target_link_libraries(SerialTest PRIVATE windowsapp)
endif()

set_target_properties(
    SerialTest
    PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "SerialTest"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.insomnia1437.serialtest"
        MACOSX_BUNDLE_ICON_FILE "icon.icns"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        WIN32_EXECUTABLE TRUE
)

install(
    TARGETS SerialTest
    BUNDLE DESTINATION .
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(BUILD_TESTING)
    add_subdirectory(tests)
    add_test(
        NAME serialtest_ui_smoke
        COMMAND SerialTest
            --config-path "${CMAKE_CURRENT_BINARY_DIR}/ui-smoke.ini"
            --screenshot "${CMAKE_CURRENT_BINARY_DIR}/ui-smoke.png"
    )
    set_tests_properties(
        serialtest_ui_smoke
        PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            TIMEOUT 20
    )

    add_test(
        NAME serialtest_detachable_panels
        COMMAND SerialTest
            --config-path "${CMAKE_CURRENT_BINARY_DIR}/ui-detach.ini"
            --verify-detachable-panels
    )
    set_tests_properties(
        serialtest_detachable_panels
        PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            TIMEOUT 20
    )

    add_test(
        NAME serialtest_settings_scroll
        COMMAND SerialTest
            --config-path "${CMAKE_CURRENT_BINARY_DIR}/ui-settings-scroll.ini"
            --verify-settings-scroll
    )
    set_tests_properties(
        serialtest_settings_scroll
        PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            TIMEOUT 20
    )

endif()
