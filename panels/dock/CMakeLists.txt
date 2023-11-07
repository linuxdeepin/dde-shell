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

add_library(dockpanel SHARED ${dock_sources})

target_link_libraries(dockpanel PRIVATE
    dde-shell-frame
)

ds_install_package(PACKAGE org.deepin.ds.dock TARGET dockpanel)

add_subdirectory(launcherItem)
add_subdirectory(showdesktop)
add_subdirectory(taskmanager)
