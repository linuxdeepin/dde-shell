# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(NOTIFICATION_SERVER "notificationserver")

file (GLOB SERVER_SOURCES *.cpp *.h)

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
