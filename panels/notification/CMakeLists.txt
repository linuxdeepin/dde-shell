# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(NOTIFICATION_PANEL "notificationpanel")

file (GLOB NOTIFICATION_SOURCES *.cpp *.h dbus/types/*.h dbus/types/*.cpp)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/dbus/xml/org.deepin.dde.SessionManager1.xml
    PROPERTIES
    CLASSNAME
        SessionManager1
)

qt6_add_dbus_interfaces(
    DBUS_INTERFACES
    ${CMAKE_CURRENT_SOURCE_DIR}/dbus/xml/org.deepin.dde.SessionManager1.xml
)

add_library(${NOTIFICATION_PANEL} SHARED
    ${DBUS_INTERFACES}
    ${NOTIFICATION_SOURCES}
    test.sh
)

target_link_libraries(${NOTIFICATION_PANEL} PRIVATE
    dde-shell-frame
    Qt${QT_MAJOR_VERSION}::DBus
    Qt${QT_MAJOR_VERSION}::Sql
)

#dconfig
dtk_add_config_meta_files(APPID org.deepin.dde.shell FILES configs/org.deepin.ds.notification.json)

ds_install_package(PACKAGE org.deepin.ds.notification TARGET ${NOTIFICATION_PANEL})
ds_handle_package_translation(PACKAGE org.deepin.ds.notification)
