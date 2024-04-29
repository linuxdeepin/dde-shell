# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(dock-shell-example SHARED
    item.cpp
    item.h
)

target_link_libraries(dock-shell-example PRIVATE
    dock-plugin
)

ds_install_package(PACKAGE org.deepin.ds.dock.dock-shell-example TARGET dock-shell-example)
