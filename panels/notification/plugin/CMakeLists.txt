# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set_source_files_properties(NotifyStyle.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(notificationplugin
    PLUGIN_TARGET notificationplugin
    URI org.deepin.ds.notification
    VERSION 1.0
    QML_FILES
        NotifyStyle.qml
        NotifyItemBackground.qml
        NotifyItem.qml
        NotifyItemContent.qml
        NotifyAction.qml
        SettingActionButton.qml
        OverlapIndicator.qml
        DropShadowText.qml
    OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/plugins/org/deepin/ds/notification/
)

install(TARGETS notificationplugin DESTINATION "${QML_INSTALL_DIR}/org/deepin/ds/notification/")
install(DIRECTORY ${PROJECT_BINARY_DIR}/plugins/org/deepin/ds/notification/ DESTINATION ${QML_INSTALL_DIR}/org/deepin/ds/notification/
    FILES_MATCHING PATTERN "qmldir" PATTERN "*.qmltypes"  PATTERN "*.qml" PATTERN "*.js"
)
