# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS DBus)
find_package(DDEApplicationManager REQUIRED)
find_package(yaml-cpp REQUIRED)

set_source_files_properties(
    ${DDE_APPLICATION_MANAGER_DBUS_API_DIR}/org.desktopspec.ApplicationManager1.Application.xml
    PROPERTIES  INCLUDE api/types/am.h
                CLASSNAME Application
)

set_source_files_properties(
    ${DDE_APPLICATION_MANAGER_DBUS_API_DIR}/org.desktopspec.ApplicationManager1.xml
    PROPERTIES  INCLUDE api/types/am.h
                CLASSNAME ApplicationManager
)

set_source_files_properties(
    ${DDE_APPLICATION_MANAGER_DBUS_API_DIR}/org.desktopspec.ObjectManager1.xml
    PROPERTIES  INCLUDE api/types/am.h
                CLASSNAME ObjectManager
)

qt_add_dbus_interfaces(
    DBUS_INTERFACES
    ${DDE_APPLICATION_MANAGER_DBUS_API_DIR}/org.desktopspec.ApplicationManager1.Application.xml
    ${DDE_APPLICATION_MANAGER_DBUS_API_DIR}/org.desktopspec.ApplicationManager1.xml
    ${DDE_APPLICATION_MANAGER_DBUS_API_DIR}/org.desktopspec.ObjectManager1.xml
)


add_library(dde-apps SHARED ${DBUS_INTERFACES}
    amappitem.cpp
    amappitem.h
    amappitemmodel.cpp
    amappitemmodel.h
    appgroup.cpp
    appgroup.h
    appgroupmanager.cpp
    appgroupmanager.h
    appitem.cpp
    appitem.h
    appitemmodel.cpp
    appitemmodel.h
    appsapplet.cpp
    appsapplet.h
    appsdockedhelper.cpp
    appsdockedhelper.h
    appslaunchtimes.cpp
    appslaunchtimes.h
    categoryutils.cpp
    categoryutils.h
)

target_link_libraries(dde-apps PRIVATE
    dde-shell-frame
    Qt${QT_VERSION_MAJOR}::Concurrent
    yaml-cpp
)

ds_install_package(PACKAGE org.deepin.ds.dde-apps TARGET dde-apps)
dtk_add_config_meta_files(APPID org.deepin.dde.shell FILES configs/org.deepin.ds.dde-apps.json)
