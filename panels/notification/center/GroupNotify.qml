// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notification
import org.deepin.ds.notificationcenter

NotifyItem {
    id: root
    implicitWidth: impl.implicitWidth
    implicitHeight: impl.implicitHeight

    signal collapse()

    Control {
        id: impl
        anchors.fill: parent

        contentItem: RowLayout {
            NotifyHeaderTitleText {
                text: root.appName
                Layout.alignment: Qt.AlignLeft
                Layout.leftMargin: 18
                tFont: DTK.fontManager.t5
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
            }

            SettingActionButton {
                Layout.alignment: Qt.AlignRight
                icon.name: "fold"
                onClicked: {
                    console.log("collapse")
                    root.collapse()
                }
            }
            SettingActionButton {
                Layout.alignment: Qt.AlignRight
                icon.name: "more"
                onClicked: function () {
                    console.log("group setting", root.appName)
                    let pos = mapToItem(root, Qt.point(width / 2, height))
                    root.setting(pos)
                }
            }
            AnimationSettingButton {
                Layout.alignment: Qt.AlignRight
                icon.name: "clean-group"
                text: qsTr("Clear All")
                onClicked: function () {
                    root.remove()
                }
            }
        }
    }
}
