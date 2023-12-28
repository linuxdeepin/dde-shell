# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

set(QT_MAJOR_VERSION 5)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Dtk REQUIRED COMPONENTS Gui Widget)

file(GLOB SRCS "*.h" "*.cpp" "interfaces/*.h" "utils/*.h" "utils/*.cpp")

add_executable(dockplugin-loader
    main.cpp
    widgetplugin.h
    widgetplugin.cpp
    utils/setproctitle.h
    utils/setproctitle.cpp
)

target_include_directories(dockplugin-loader PUBLIC
    "utils/"
    "interfaces/"
    "../plugin/"
)

target_link_libraries(dockplugin-loader PRIVATE
    dockpluginmanager-interface
    Qt5::Core
    Qt5::Widgets
    Dtk::Widget
    Dtk::Gui
)

install(TARGETS dockplugin-loader DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
