# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Sql)

qt_add_qml_module(notificationcenterpanel
    PLUGIN_TARGET notificationcenterpanelplugin
    URI org.deepin.ds.notificationcenter
    VERSION 1.0
    QML_FILES
        NotifyCenter.qml
        NotifyHeader.qml
        NotifyBottom.qml
        NotifyView.qml
        NotifyViewDelegate.qml
        NotifyItemBackground.qml
        NotifyItem.qml
        NormalNotify.qml
        OverlapNotify.qml
        GroupNotify.qml
        NotifyAction.qml
        NotifySetting.qml
        NotifySettingMenu.qml
        SettingActionButton.qml
        BoundingRectangle.qml
        DataPanel.qml
    SOURCES
        notificationcenterpanel.h
        notificationcenterpanel.cpp
        notificationcenterproxy.h
        notificationcenterproxy.cpp
        notifyaccessor.h
        notifyaccessor.cpp
        notifymodel.h
        notifymodel.cpp
        notifyentity.h
        notifyentity.cpp
        notifyitem.h
        notifyitem.cpp
        dbaccessor.h
        dbaccessor.cpp
    OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/plugins/org/deepin/ds/notificationcenter/
)

target_link_libraries(notificationcenterpanel
    PRIVATE
        dde-shell-frame
        Qt${QT_VERSION_MAJOR}::Quick
        Qt${QT_VERSION_MAJOR}::DBus
        Qt${QT_VERSION_MAJOR}::Sql
)

install(TARGETS notificationcenterpanelplugin DESTINATION "${QML_INSTALL_DIR}/org/deepin/ds/notificationcenter/")
install(DIRECTORY "${PROJECT_BINARY_DIR}/plugins/org/deepin/ds/notificationcenter/" DESTINATION "${QML_INSTALL_DIR}/org/deepin/ds/notificationcenter/")

ds_install_package(PACKAGE org.deepin.ds.notificationcenter TARGET notificationcenterpanel)
ds_handle_package_translation(PACKAGE org.deepin.ds.notificationcenter)
