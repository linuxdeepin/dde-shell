# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} COMPONENTS DBus)

file(
    GLOB dock_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Old dbus interface compatible
qt_add_dbus_adaptor(dock_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/api/old/org.deepin.dde.dock1.xml
    dockdbusproxy.h
    ds::dock::DockDBusProxy
    dockfrontadaptor
    DockFrontAdaptor
)
qt_add_dbus_adaptor(dock_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/api/old/org.deepin.dde.daemon.Dock1.xml
    dockdbusproxy.h
    ds::dock::DockDBusProxy
    dockdaemonadaptor
    DockDaemonAdaptor
)
# dock dbus api
qt_add_dbus_adaptor(dock_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/api/dbus/org.deepin.ds.dock.xml
    dockpanel.h
    ds::dock::DockPanel
    dockadaptor
    DockAdaptor
)

set_source_files_properties(MenuHelper.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_policy(SET QTP0001 OLD)
qt_add_qml_module(dockpanel
    PLUGIN_TARGET dockpanel-plugin
    URI "org.deepin.ds.dock"
    VERSION "1.0"
    SHARED
    SOURCES ${dock_sources}
    QML_FILES OverflowContainer.qml MenuHelper.qml
    OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/plugins/org/deepin/ds/dock
)

target_link_libraries(dockpanel PRIVATE
    dde-shell-frame
)

ds_install_package(PACKAGE org.deepin.ds.dock TARGET dockpanel)

add_subdirectory(launcherItem)
add_subdirectory(showdesktop)
add_subdirectory(taskmanager)

install(DIRECTORY "${PROJECT_BINARY_DIR}/plugins/org/deepin/ds/dock/" DESTINATION "${QML_INSTALL_DIR}/org/deepin/ds/dock/")
