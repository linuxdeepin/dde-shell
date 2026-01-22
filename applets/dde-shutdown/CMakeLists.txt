# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later
find_package(TreelandProtocols REQUIRED)

add_library(dde-shutdown SHARED
    shutdownapplet.cpp
    shutdownapplet.h
    treelandlockscreen.h
    treelandlockscreen.cpp
)

qt_generate_wayland_protocol_client_sources(dde-shutdown
    NO_INCLUDE_CORE_ONLY
    FILES
        ${TREELAND_PROTOCOLS_DATA_DIR}/treeland-dde-shell-v1.xml
)

target_link_libraries(dde-shutdown PRIVATE
    dde-shell-frame
    Qt${QT_MAJOR_VERSION}::DBus
    Qt${QT_VERSION_MAJOR}::WaylandClient
)

ds_install_package(PACKAGE org.deepin.ds.dde-shutdown TARGET dde-shutdown)
