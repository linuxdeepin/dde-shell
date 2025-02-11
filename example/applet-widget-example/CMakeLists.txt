# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Widgets)
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget)

add_library(ds-example-applet-widget SHARED
    exampleapplet.h
    exampleapplet.cpp
)

target_link_libraries(ds-example-applet-widget PRIVATE
    dde-shell-frame
    Qt${QT_VERSION_MAJOR}::Widgets
    Dtk${DTK_VERSION_MAJOR}::Widget
)

ds_install_package(PACKAGE org.deepin.ds.example.applet-widget TARGET ds-example-applet-widget)
