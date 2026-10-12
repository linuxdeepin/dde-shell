# SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(PkgConfig REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} COMPONENTS WaylandClient)
find_package(TreelandProtocols 0.7.0 REQUIRED)
pkg_check_modules(WaylandClient REQUIRED IMPORTED_TARGET wayland-client)

add_library(dock-multitaskview SHARED
    multitaskview.cpp
    multitaskview.h
    treelandcompositoraction.cpp
    treelandcompositoraction.h
)

target_include_directories(dock-multitaskview PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/../"
    "../"
)

qt_generate_wayland_protocol_client_sources(dock-multitaskview
    FILES
        ${TREELAND_PROTOCOLS_DATA_DIR}/treeland-compositor-action-unstable-v1.xml
)

target_link_libraries(dock-multitaskview PRIVATE
    PkgConfig::WaylandClient
    Qt${QT_VERSION_MAJOR}::WaylandClient
    dde-shell-frame
    dde-shell-dock
)

if (HAVE_DDE_API_EVENTLOGGER)
    target_compile_definitions(dock-multitaskview PRIVATE HAVE_DDE_API_EVENTLOGGER)
    target_link_libraries(dock-multitaskview PRIVATE DDEAPI::EventLogger)
endif()

ds_install_package(PACKAGE org.deepin.ds.dock.multitaskview TARGET dock-multitaskview)
ds_handle_package_translation(PACKAGE org.deepin.ds.dock.multitaskview)

install(FILES "package/icons/dcc-view.dci" DESTINATION share/dde-dock/icons/dcc-setting RENAME dcc-multitasking-view.dci)
