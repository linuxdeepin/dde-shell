// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notificationcenter

NotifyItem {
    id: root

    signal collapse()

    contentItem: RowLayout {
        Text {
            text: root.appName
            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: 18
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
        }

        SettingActionButton {
            Layout.alignment: Qt.AlignRight
            icon.name: "collapse"
            onClicked: {
                console.log("collapse")
                root.collapse()
            }
        }
        SettingActionButton {
            Layout.alignment: Qt.AlignRight
            icon.name: "settings"
            onClicked: function () {
                console.log("group setting", appName)
                let pos = mapToItem(root, Qt.point(width / 2, height))
                root.setting(pos)
            }
        }
        AnimationSettingButton {
            Layout.alignment: Qt.AlignRight
            icon.name: "close"
            text: qsTr("Clear All")
            onClicked: function () {
                root.remove()
            }
        }
    }
}
