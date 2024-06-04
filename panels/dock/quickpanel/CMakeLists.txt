# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

qt_add_qml_module(dock-quickpanel
    PLUGIN_TARGET dock-quickpanel-plugin
    URI "org.deepin.ds.dock.quickpanel"
    VERSION "1.0"
    SHARED
    SOURCES
        quickpanel.cpp
        quickpanel.h
        quickpanelproxymodel.cpp
        quickpanelproxymodel.h
    OUTPUT_DIRECTORY
        "${PROJECT_BINARY_DIR}/plugins/org/deepin/ds/dock/quickpanel"
)

target_link_libraries(dock-quickpanel PRIVATE
    dde-shell-frame
)

ds_install_package(PACKAGE org.deepin.ds.dock.quickpanel TARGET dock-quickpanel)
