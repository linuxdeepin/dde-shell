# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} COMPONENTS DBus WaylandClient)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/api/amdbus/org.desktopspec.ApplicationManager1.Application.xml
    PROPERTIES  INCLUDE api/types/am.h
                CLASSNAME Application
)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/api/amdbus/org.desktopspec.ApplicationManager1.xml
    PROPERTIES  INCLUDE api/types/am.h
                CLASSNAME ApplicationManager
)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/api/amdbus/org.desktopspec.ProcessGuesser1.xml
    PROPERTIES  INCLUDE api/types/am.h
                CLASSNAME ProcessGuesser
)

qt_add_dbus_interfaces(
    DBUS_INTERFACES
    ${CMAKE_CURRENT_SOURCE_DIR}/api/amdbus/org.desktopspec.ApplicationManager1.Application.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/api/amdbus/org.desktopspec.ApplicationManager1.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/api/amdbus/org.desktopspec.ProcessGuesser1.xml
)

qt_add_dbus_adaptor(DBUS_INTERFACES
    ${CMAKE_CURRENT_SOURCE_DIR}/api/dbus/org.deepin.ds.dock.taskmanager.item.xml
    appitem.h
    ds::dock::AppItem
    appitemadaptor
    AppItemAdaptor
)

qt_add_dbus_adaptor(DBUS_INTERFACES
    ${CMAKE_CURRENT_SOURCE_DIR}/api/dbus/org.deepin.ds.dock.taskmanager.xml
    taskmanager.h
    ds::dock::TaskManager
    taskmanageradaptor
    TaskManagerAdaptor
)

add_library(dock-taskmanager SHARED ${DBUS_INTERFACES}
    abstractwindow.h
    abstractwindowmonitor.h
    appitem.cpp
    appitem.h
    appitemmodel.cpp
    appitemmodel.h
    desktopfileabstractparser.cpp
    desktopfileabstractparser.h
    desktopfileamparser.cpp
    desktopfileamparser.h
    desktopfilenoneparser.cpp
    desktopfilenoneparser.h
    desktopfileparserfactory.h
    taskmanager.cpp
    taskmanager.h
    waylandwindow.cpp
    waylandwindow.h
    waylandwindowmonitor.cpp
    waylandwindowmonitor.h
    taskmanagersettings.cpp
    taskmanagersettings.h
    desktopfileamparsersettings.h
    desktopfileamparsersettings.cpp
)

qt_generate_wayland_protocol_client_sources(dock-taskmanager
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/api/wayland-protocols/treeland-foreign-toplevel-manager-v1.xml
        ${WaylandProtocols_DATADIR}/staging/ext-foreign-toplevel-list/ext-foreign-toplevel-list-v1.xml
)

target_link_libraries(dock-taskmanager PRIVATE
    dde-shell-frame
    Qt${QT_VERSION_MAJOR}::WaylandClientPrivate
    PkgConfig::WaylandClient
)

if (BUILD_WITH_X11)
    target_compile_definitions(dock-taskmanager PRIVATE BUILD_WITH_X11=)
    pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb xcb-res xcb-ewmh)
    target_sources(dock-taskmanager PRIVATE
        x11utils.cpp
        x11utils.h
        x11window.cpp
        x11window.h
        x11windowmonitor.cpp
        x11windowmonitor.h
    )

    target_link_libraries(dock-taskmanager PUBLIC PkgConfig::XCB)
endif(BUILD_WITH_X11)

ds_install_package(PACKAGE org.deepin.ds.dock.taskmanager TARGET dock-taskmanager)
