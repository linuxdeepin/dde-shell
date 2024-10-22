# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later
find_package(Dtk6 REQUIRED COMPONENTS Core Tools)

set(BUBBLE_PANEL "notificationbubble")

file (GLOB BUBBLE_SOURCES *.cpp *.h)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/dbus/xml/org.deepin.dde.SessionManager1.xml
    PROPERTIES
    CLASSNAME
        BubbleSessionManager1
)

dtk_add_dbus_interface(
    DBUS_INTERFACES
    ${CMAKE_CURRENT_SOURCE_DIR}/dbus/xml/org.deepin.dde.SessionManager1.xml
    sessionmanager1interface
)

add_library(${BUBBLE_PANEL} SHARED
    ${DBUS_INTERFACES}
    ${BUBBLE_SOURCES}
)

target_link_libraries(${BUBBLE_PANEL} PRIVATE
    dde-shell-frame
    ds-notification-shared
    Qt${QT_MAJOR_VERSION}::DBus
)

ds_install_package(PACKAGE org.deepin.ds.notificationbubble TARGET ${BUBBLE_PANEL})
ds_handle_package_translation(PACKAGE org.deepin.ds.notificationbubble)
