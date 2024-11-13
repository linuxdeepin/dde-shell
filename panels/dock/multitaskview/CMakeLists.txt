# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(PkgConfig REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} COMPONENTS WaylandClient)
find_package(TreelandProtocols REQUIRED)
pkg_check_modules(WaylandClient REQUIRED IMPORTED_TARGET wayland-client)

add_library(dock-multitaskview SHARED
    multitaskview.cpp
    multitaskview.h
    treelandmultitaskview.cpp
    treelandmultitaskview.h
    ../dockiteminfo.cpp
    ../dockiteminfo.h
)

target_include_directories(dock-multitaskview PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/../"
    "../"
)

qt_generate_wayland_protocol_client_sources(dock-multitaskview
    FILES
        ${TREELAND_PROTOCOLS_DATA_DIR}/treeland-dde-shell-v1.xml
)

target_link_libraries(dock-multitaskview PRIVATE
    PkgConfig::WaylandClient
    Qt${QT_VERSION_MAJOR}::WaylandClient
    dde-shell-frame
)

ds_install_package(PACKAGE org.deepin.ds.dock.multitaskview TARGET dock-multitaskview)
ds_handle_package_translation(PACKAGE org.deepin.ds.dock.multitaskview)

install(FILES "package/icons/multitasking-view.svg" DESTINATION share/dde-dock/icons/dcc-setting)
