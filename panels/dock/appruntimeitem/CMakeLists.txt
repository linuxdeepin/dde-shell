# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

if (BUILD_WITH_X11)
find_package(XCB REQUIRED)
add_library(dock-appruntimeitem SHARED
    appruntimeitem.cpp
    appruntimeitem.h
    windowmanager.cpp
    windowmanager.h
    xcbgetinfo.h
    xcbgetinfo.cpp
    qmlappruntime.qrc
    ${CMAKE_SOURCE_DIR}/panels/dock/dockiteminfo.cpp
    ${CMAKE_SOURCE_DIR}/panels/dock/dockiteminfo.h
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/x11utils.h
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/x11utils.cpp
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/abstractwindow.h
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/x11window.h
    ${CMAKE_SOURCE_DIR}/panels/dock/taskmanager/x11window.cpp

    #${CMAKE_SOURCE_DIR}/panels/dock/constants.h
)

include_directories(${CMAKE_SOURCE_DIR}/panels/dock)
include_directories(${CMAKE_SOURCE_DIR}/panels/dock/taskmanager)

target_include_directories(dock-appruntimeitem PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/../"
    "${CMAKE_SOURCE_DIR}/panels/dock"
)


target_link_libraries(dock-appruntimeitem PRIVATE
    dde-shell-frame
    ${XCB_LIBRARIES}
)
target_link_libraries(dock-appruntimeitem PRIVATE
    dde-shell-frame
)

ds_install_package(PACKAGE org.deepin.ds.dock.appruntimeitem TARGET dock-appruntimeitem)
ds_handle_package_translation(PACKAGE org.deepin.ds.dock.appruntimeitem)

install(FILES "package/icons/appruntime.svg" DESTINATION share/dde-dock/icons/dcc-setting)
endif(BUILD_WITH_X11)
