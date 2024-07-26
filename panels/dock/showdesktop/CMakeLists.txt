# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(dock-showdesktop SHARED
    showdesktop.cpp
    showdesktop.h
    ../dockiteminfo.cpp
    ../dockiteminfo.h
)

target_include_directories(dock-showdesktop PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/../"
    "../"
)

target_link_libraries(dock-showdesktop PRIVATE
    dde-shell-frame
)

ds_install_package(PACKAGE org.deepin.ds.dock.showdesktop TARGET dock-showdesktop)
ds_handle_package_translation(PACKAGE org.deepin.ds.dock.showdesktop)

install(FILES "package/icons/showdesktop.svg" DESTINATION share/dde-dock/icons/dcc-setting)
