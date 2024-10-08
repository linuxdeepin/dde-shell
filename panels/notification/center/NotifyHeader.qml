// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notificationcenter

FocusScope {
    id: root

    required property NotifyModel notifyModel
    signal headerClicked()

    NotifySettingMenu {
        id: notifySetting
        MenuItem {
            text: qsTr("Notification Setting")
            onClicked: {
                console.log("Notify setting")
                NotifyAccessor.openNotificationSetting()
            }
        }
    }

    // test
    onHeaderClicked: function () {
        dataPanel.show()
    }
    Window {
        id: dataPanel
        width: 360
        height: 600
        x: dataPanel.transientParent.x + root.Window.width + 10
        y: dataPanel.transientParent.y
        DataPanel {
            notifyModel: root.notifyModel
        }
    }

    ColumnLayout {
        objectName: "notificationHeader"
        anchors.fill: parent
        focus: false
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Notification Center")
                Layout.alignment: Qt.AlignLeft
                Layout.leftMargin: 18
                font {
                    pixelSize: DTK.fontManager.t4.pixelSize
                    family: DTK.fontManager.t4.family
                    bold: true
                }
                color: palette.windowText
                MouseArea {
                    anchors.fill: parent
                    onDoubleClicked: {
                        root.headerClicked()
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
            }

            SettingActionButton {
                objectName: "collapse"
                focus: true
                visible: !notifyModel.collapse
                Layout.alignment: Qt.AlignRight
                icon.name: "fold"
                onClicked: function () {
                    console.log("Clear all notify")
                    notifyModel.collapseAllApp()
                }
            }

            SettingActionButton {
                objectName: "more"
                focus: true
                Layout.alignment: Qt.AlignRight
                icon.name: "more"
                onClicked: function () {
                    console.log("Setting notify")
                    let pos = mapToItem(root, Qt.point(width / 2, height))
                    notifySetting.x = pos.x - notifySetting.width / 2
                    notifySetting.y = pos.y

                    notifySetting.toggle()
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
}
