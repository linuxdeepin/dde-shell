# SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Network)

add_library(dde-weather SHARED
    weatherapplet.h
    weatherapplet.cpp
    weathermodel.h
    weathermodel.cpp
    weatherhelper.h
    weatherhelper.cpp
)

target_link_libraries(dde-weather PRIVATE
    dde-shell-frame
    Qt${QT_VERSION_MAJOR}::Network
)

find_package(Dtk${DTK_VERSION_MAJOR}DConfig REQUIRED)

dtk_add_config_meta_files(APPID "org.deepin.dde.shell" FILES configs/org.deepin.ds.weather.json)

ds_install_package(PACKAGE org.deepin.ds.weather TARGET dde-weather)

ds_handle_package_translation(PACKAGE org.deepin.ds.weather)
