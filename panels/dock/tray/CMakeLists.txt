# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

add_subdirectory(quickpanel)

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Gui Qml)

add_library(dock-traymodel SHARED
    traysortordermodel.cpp
    traysortordermodel.h
)

target_link_libraries(dock-traymodel PRIVATE
    Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Qml
)

ds_install_package(PACKAGE org.deepin.ds.dock.tray)

qt_policy(SET QTP0001 OLD)
qt_add_qml_module(dock-tray
    PLUGIN_TARGET dock-tray
    URI "org.deepin.ds.dock.tray"
    VERSION "1.0"
    SHARED
    SOURCES
        traysortordermodel.cpp
        traysortordermodel.h
    OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/plugins/org/deepin/ds/dock/tray/
)
target_link_libraries(dock-tray PUBLIC
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
)