# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

configure_file(environments.h.in environments.h @ONLY)

add_library(docktray SHARED
    docktray.cpp
    docktray.h
)

target_link_libraries(docktray PRIVATE
    dde-shell-frame
)

ds_install_package(PACKAGE org.deepin.ds.dock.tray TARGET docktray)

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Gui Qml WaylandCompositor Widgets)

file(
    GLOB dock_plugin_compositor_extenstion_files
    dockpluginmanagerextension_p.h
    dockpluginmanagerextension.cpp
    dockpluginmanagerintegration_p.h
    dockpluginmanagerintegration.cpp
)

set_source_files_properties(DockCompositor.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(dock-plugin-compositor
    PLUGIN_TARGET dock-plugin-compositor
    URI "org.deepin.ds.dock.compositor"
    VERSION "1.0"
    SHARED
    SOURCES ${dock_plugin_compositor_extenstion_files}
    QML_FILES DockCompositor.qml
    OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/plugins/org/deepin/ds/dock/compositor/
)

qt_generate_wayland_protocol_server_sources(dock-plugin-compositor
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/protocol/dock-plugin-manager-v1.xml
)

target_link_libraries(dock-plugin-compositor PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Widgets
    Qt::WaylandCompositor
PRIVATE
    Qt::WaylandCompositorPrivate
)

add_subdirectory(client)
