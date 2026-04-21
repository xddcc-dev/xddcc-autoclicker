cmake_minimum_required(VERSION 3.16)
project(XDDCCAutoClicker VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Multimedia)

qt_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/clickengine.cpp
    src/thememanager.cpp
    src/settingsdialog.cpp
    src/windowpicker.cpp
    src/starburstwidget.cpp
    src/themeeditor.cpp
    src/particlewidget.cpp
    src/positionpicker.cpp
)

set(HEADERS
    src/mainwindow.h
    src/clickengine.h
    src/thememanager.h
    src/settingsdialog.h
    src/windowpicker.h
    src/starburstwidget.h
    src/themeeditor.h
    src/particlewidget.h
    src/positionpicker.h
)

# On Windows: WIN32 suppresses the console window
if(WIN32)
    qt_add_executable(XDDCCAutoClicker WIN32 ${SOURCES} ${HEADERS} resources/resources.qrc)
else()
    qt_add_executable(XDDCCAutoClicker ${SOURCES} ${HEADERS} resources/resources.qrc)
endif()

target_link_libraries(XDDCCAutoClicker PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
)

# Platform-specific click simulation libraries
if(WIN32)
    target_link_libraries(XDDCCAutoClicker PRIVATE user32 winmm psapi)
elseif(UNIX AND NOT APPLE)
    # X11 + XTest for mouse simulation on Linux
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(XTST REQUIRED xtst)
    pkg_check_modules(X11  REQUIRED x11)
    target_include_directories(XDDCCAutoClicker PRIVATE ${XTST_INCLUDE_DIRS} ${X11_INCLUDE_DIRS})
    target_link_libraries(XDDCCAutoClicker PRIVATE ${XTST_LIBRARIES} ${X11_LIBRARIES})
    target_compile_options(XDDCCAutoClicker PRIVATE ${XTST_CFLAGS_OTHER})
endif()
