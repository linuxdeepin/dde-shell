# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(XCB REQUIRED)
add_library(dock-appruntimeitem SHARED
    appruntimeitem.cpp
    appruntimeitem.h
    windowmanager.cpp
    windowmanager.h
    xcbthread.cpp
    xcbthread.h
    xcb_get.h
    xcb_get.cpp
    qmlappruntime.qrc
    ../dockiteminfo.cpp
    ../dockiteminfo.h
)

target_include_directories(dock-appruntimeitem PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/../"
    "../"
)
# 链接 XCB 库
target_link_libraries(dock-appruntimeitem PRIVATE
    dde-shell-frame
    ${XCB_LIBRARIES}  # 添加 XCB 库
)
target_link_libraries(dock-appruntimeitem PRIVATE
    dde-shell-frame
)

ds_install_package(PACKAGE org.deepin.ds.dock.appruntimeitem TARGET dock-appruntimeitem)
ds_handle_package_translation(PACKAGE org.deepin.ds.dock.appruntimeitem)

install(FILES "package/icons/appruntime.svg" DESTINATION share/dde-dock/icons/dcc-setting)

