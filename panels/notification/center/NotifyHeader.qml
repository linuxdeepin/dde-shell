// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notification
import org.deepin.ds.notificationcenter

FocusScope {
    id: root

    required property NotifyModel notifyModel

    RowLayout {
        anchors.fill: parent
        NotifyHeaderTitleText {
            text: qsTr("Notification Center")
            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: 18
            tFont: DTK.fontManager.t4
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
        }

        AnimationSettingButton {
            objectName: "collapse"
            focus: true
            visible: !notifyModel.collapse
            Layout.alignment: Qt.AlignRight
            icon.name: "fold"
            text: qsTr("Fold")
            onClicked: function () {
                console.log("Collapse all notify")
                notifyModel.collapseAllApp()
            }
        }

        AnimationSettingButton {
            objectName: "more"
            focus: true
            Layout.alignment: Qt.AlignRight
            icon.name: "more"
            text: qsTr("More")
            onClicked: function () {
                console.log("Notify setting")
                NotifyAccessor.openNotificationSetting()
            }
        }

        AnimationSettingButton {
            objectName: "closeAllNotify"
            icon.name: "clean-all"
            text: qsTr("Clear All")
            Layout.alignment: Qt.AlignRight
            onClicked: function () {
                console.log("Clear all notify")
                notifyModel.clear()
            }
        }
    }
}
