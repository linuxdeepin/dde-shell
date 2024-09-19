// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.ds.notificationcenter

Control {
    id: root

    property var actions: []
    signal actionInvoked(var actionId)

    topPadding: 8
    bottomPadding: 8
    leftPadding: 4
    rightPadding: 4
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
        required property var actionData
        text: actionData.text
        implicitHeight: 30
        implicitWidth: 50
        padding: 0
        font: DTK.fontManager.t6
        onClicked: {
            console.log("action invoked", actionData.id)
            actionInvoked(actionData.id)
        }
    }
}
