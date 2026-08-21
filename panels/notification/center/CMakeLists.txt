# SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

# The QML module is its own plugin.  Its implementation does not link to the
# dde-shell applet entry point.
qt_add_qml_module(notificationcenterpanelplugin
    SHARED
    PLUGIN_TARGET notificationcenterpanelplugin
    URI org.deepin.ds.notificationcenter
    VERSION 1.0
    SOURCES
        notifyaccessor.h
        notifyaccessor.cpp
        notifymodel.h
        notifymodel.cpp
        notifyitem.h
        notifyitem.cpp
        notifystagingmodel.h
        notifystagingmodel.cpp
    QML_FILES
        NotifyCenter.qml
        NotifyStaging.qml
        NotifyHeader.qml
        NotifyView.qml
        NotifyViewDelegate.qml
        NormalNotify.qml
        OverlapNotify.qml
        GroupNotify.qml
        NotifySetting.qml
        NotifySettingMenu.qml
        AnimationSettingButton.qml
        BoundingRectangle.qml
        NotifyHeaderTitleText.qml
    OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/plugins/org/deepin/ds/notificationcenter/
)

qt_add_resources(notificationcenterpanelplugin "ds_notificationcenterpanel_icons"
    PREFIX "/dsg/built-in-icons"
    BASE "icons"
    FILES
        icons/clean-group.dci
        icons/clean-alone.dci
        icons/clean-all.dci
        icons/fold.dci
        icons/more.dci
)

target_link_libraries(notificationcenterpanelplugin
    PRIVATE
        dde-shell-frame
        ds-notification-shared
        Qt${QT_VERSION_MAJOR}::Quick
        Qt${QT_VERSION_MAJOR}::DBus
)

add_library(notificationcenterapplet SHARED
    notificationcenterpanel.h
    notificationcenterpanel.cpp
    notificationcenterproxy.h
    notificationcenterproxy.cpp
    notificationcenterdbusadaptor.h
    notificationcenterdbusadaptor.cpp
)

target_link_libraries(notificationcenterapplet
    PRIVATE
        dde-shell-frame
        ds-notification-shared
        Qt${QT_VERSION_MAJOR}::DBus
)

install(TARGETS notificationcenterpanelplugin DESTINATION "${QML_INSTALL_DIR}/org/deepin/ds/notificationcenter/")
install(DIRECTORY ${PROJECT_BINARY_DIR}/plugins/org/deepin/ds/notificationcenter/ DESTINATION ${QML_INSTALL_DIR}/org/deepin/ds/notificationcenter/
    FILES_MATCHING PATTERN "qmldir" PATTERN "*.qmltypes" PATTERN "*.qml" PATTERN "*.js"
)

ds_install_package(PACKAGE org.deepin.ds.notificationcenter TARGET notificationcenterapplet)
ds_handle_package_translation(PACKAGE org.deepin.ds.notificationcenter)
