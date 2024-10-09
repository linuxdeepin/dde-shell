# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(NOTIFICATION_SERVER "notificationserver")

file (GLOB SERVER_SOURCES *.cpp *.h)

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

add_library(${NOTIFICATION_SERVER} SHARED
    ${DBUS_INTERFACES}
    ${SERVER_SOURCES}
)

target_link_libraries(${NOTIFICATION_SERVER} PRIVATE
    dde-shell-frame
    ds-notification-shared
    Qt${QT_MAJOR_VERSION}::DBus
)

dtk_add_config_meta_files(APPID org.deepin.dde.shell FILES configs/org.deepin.dde.shell.notification.json)
ds_install_package(PACKAGE org.deepin.ds.notificationserver TARGET ${NOTIFICATION_SERVER})
