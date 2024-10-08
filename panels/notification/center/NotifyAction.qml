// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.dtk.private 1.0 as DP
import org.deepin.ds.notificationcenter

Control {
    id: root

    property var actions: []
    signal actionInvoked(var actionId)

    contentItem: RowLayout {
        spacing: 5
        height: 30
        NotifyActionButton {
            actionData: actions[0]
            Layout.maximumWidth: 60
            Layout.preferredHeight: 30
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
        }

        Loader {
            active: actions.length === 2
            visible: active
            Layout.maximumWidth: 60
            Layout.preferredHeight: 30
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
            sourceComponent: NotifyActionButton {
                actionData: actions[1]
            }
        }

        Loader {
            active: actions.length > 2
            visible: active
            Layout.maximumWidth: 80
            Layout.alignment: Qt.AlignHCenter
            sourceComponent: ComboBox {
                property var expandActions: actions.slice(1)
                textRole: "text"
                padding: 0
                implicitHeight: 30
                implicitWidth: 60
                model: expandActions
                delegate: NotifyActionButton {
                    required property int index

                    actionData: expandActions[index]
                }
            }
        }
    }

    component NotifyActionButton: Button {
        id: actionButton
        required property var actionData
        text: actionData.text
        topPadding: undefined
        bottomPadding: undefined
        leftPadding: undefined
        rightPadding: undefined
        padding: 0
        spacing: 0
        font: DTK.fontManager.t6
        onClicked: {
            console.log("action invoked", actionData.id)
            actionInvoked(actionData.id)
        }

        background: DP.ButtonPanel {
            implicitHeight: 30
            implicitWidth: 50
            button: actionButton
            color1: Palette {
                normal {
                    common: ("transparent")
                    crystal: Qt.rgba(0 / 255.0, 0 / 255.0, 0 / 255.0, 0.15)
                }
                normalDark {
                    crystal: Qt.rgba(24 / 255.0, 24 / 255.0, 24 / 255.0, 1)
                }
            }
            color2: color1
            insideBorderColor: null
            outsideBorderColor: null
        }
    }
}
