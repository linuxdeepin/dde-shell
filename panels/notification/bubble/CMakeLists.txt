# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(BUBBLE_PANEL "notificationbubble")

file (GLOB BUBBLE_SOURCES *.cpp *.h)

add_library(${BUBBLE_PANEL} SHARED
    ${BUBBLE_SOURCES}
)

target_include_directories(${BUBBLE_PANEL} PRIVATE
    "../common"
)

target_link_libraries(${BUBBLE_PANEL} PRIVATE
    dde-shell-frame
    notification-shared
)

ds_install_package(PACKAGE org.deepin.ds.notificationbubble TARGET ${BUBBLE_PANEL})
ds_handle_package_translation(PACKAGE org.deepin.ds.notification.bubble)
