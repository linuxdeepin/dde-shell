# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(dock-searchitem SHARED
    searchitem.cpp
    searchitem.h
)

target_link_libraries(dock-searchitem PRIVATE
    dock-plugin
)

ds_install_package(PACKAGE org.deepin.ds.dock.searchitem TARGET dock-searchitem)
ds_handle_package_translation(PACKAGE org.deepin.ds.dock.searchitem)
