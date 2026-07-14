# SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(TreelandProtocols REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WaylandClient REQUIRED IMPORTED_TARGET wayland-client)

add_library(osd-brightness SHARED
    brightnessapplet.cpp
    brightnessapplet.h
    treelandbrightness.cpp
    treelandbrightness.h
)

qt_generate_wayland_protocol_client_sources(osd-brightness
    NO_INCLUDE_CORE_ONLY
    FILES
        ${TREELAND_PROTOCOLS_DATA_DIR}/treeland-output-manager-v1.xml
)

target_link_libraries(osd-brightness PRIVATE
    dde-shell-frame
    Qt${QT_MAJOR_VERSION}::DBus
    Qt${QT_MAJOR_VERSION}::Gui
    Qt${QT_MAJOR_VERSION}::WaylandClient
    PkgConfig::WaylandClient
)

ds_install_package(PACKAGE org.deepin.ds.osd.brightness TARGET osd-brightness)
