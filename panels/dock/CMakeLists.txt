# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

configure_file(environments.h.in environments.h @ONLY)

find_package(PkgConfig REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} COMPONENTS Core DBus Gui Qml WaylandCompositor Widgets)
pkg_check_modules(WaylandClient REQUIRED IMPORTED_TARGET wayland-client)

file(
    GLOB dock_panel_sources
    constants.h
    dockabstractsettingsconfig.h
    dockdbusproxy.cpp
    dockdbusproxy.h
    dockpanel.cpp
    dockpanel.h
    docksettings.cpp
    docksettings.h
)

# Old dbus interface compatible
qt_add_dbus_adaptor(dock_panel_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/api/old/org.deepin.dde.dock1.xml
    dockdbusproxy.h
    ds::dock::DockDBusProxy
    dockfrontadaptor
    DockFrontAdaptor
)
qt_add_dbus_adaptor(dock_panel_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/api/old/org.deepin.dde.daemon.Dock1.xml
    dockdbusproxy.h
    ds::dock::DockDBusProxy
    dockdaemonadaptor
    DockDaemonAdaptor
)
# dock dbus api
qt_add_dbus_adaptor(dock_panel_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/api/dbus/org.deepin.ds.dock.xml
    dockpanel.h
    ds::dock::DockPanel
    dockadaptor
    DockAdaptor
)

add_library(dockpanel SHARED
    ${dock_panel_sources}
)

target_link_libraries(dockpanel PRIVATE
    PkgConfig::WaylandClient
    dde-shell-frame
)

ds_install_package(PACKAGE org.deepin.ds.dock TARGET dockpanel)

# sub plugins
add_subdirectory(clipboarditem)
add_subdirectory(workspaceitem)
add_subdirectory(searchitem)
add_subdirectory(launcherItem)
add_subdirectory(showdesktop)
add_subdirectory(slidingpanel)
add_subdirectory(taskmanager)
add_subdirectory(tray)

# dock plugin load program
add_subdirectory(dockplugin)

# dock qml element(include Dock.xx defines and DockCompositor)
file(
    GLOB dock_plugin_sources
    constants.h
    # dockfilterproxymodel.cpp
    # dockfilterproxymodel.h
    dockpluginmanagerextension_p.h
    dockpluginmanagerextension.cpp
    dockpluginmanagerintegration_p.h
    dockpluginmanagerintegration.cpp
)

set_source_files_properties(DockCompositor.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

set_source_files_properties(MenuHelper.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_policy(SET QTP0001 OLD)
qt_add_qml_module(dock-plugin
    PLUGIN_TARGET dock-plugin
    URI "org.deepin.ds.dock"
    VERSION "1.0"
    SHARED
    SOURCES ${dock_plugin_sources}
    QML_FILES DockCompositor.qml OverflowContainer.qml MenuHelper.qml
    OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/plugins/org/deepin/ds/dock/
)

qt_generate_wayland_protocol_server_sources(dock-plugin
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/dockplugin/protocol/dock-plugin-manager-v1.xml
)

target_link_libraries(dock-plugin PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::WaylandCompositor
PRIVATE
    PkgConfig::WaylandClient
    Qt${QT_VERSION_MAJOR}::WaylandCompositorPrivate
)

target_include_directories(dock-plugin
    PUBLIC
    $<TARGET_PROPERTY:dde-shell-frame,INTERFACE_INCLUDE_DIRECTORIES>
)

install(DIRECTORY "${PROJECT_BINARY_DIR}/plugins/org/deepin/ds/dock/" DESTINATION "${QML_INSTALL_DIR}/org/deepin/ds/dock/")
dtk_add_config_meta_files(APPID org.deepin.ds.dock FILES dconfig/org.deepin.ds.dock.json)
