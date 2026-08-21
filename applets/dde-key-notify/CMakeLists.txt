# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(TreelandProtocols REQUIRED)

set(TREELAND_KEYBOARD_STATE_NOTIFY_PROTOCOL
    ${TREELAND_PROTOCOLS_DATA_DIR}/treeland-keyboard-state-notify-unstable-v1.xml
)

add_library(dde-key-notify SHARED
    keynotifyapplet.cpp
    keynotifyapplet.h
    treelandkeynotify.cpp
    treelandkeynotify.h
)

qt_generate_wayland_protocol_client_sources(dde-key-notify
    NO_INCLUDE_CORE_ONLY
    FILES
        ${TREELAND_KEYBOARD_STATE_NOTIFY_PROTOCOL}
)

target_link_libraries(dde-key-notify PRIVATE
    dde-shell-frame
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::WaylandClient
)

ds_install_package(PACKAGE org.deepin.ds.dde-key-notify TARGET dde-key-notify)
