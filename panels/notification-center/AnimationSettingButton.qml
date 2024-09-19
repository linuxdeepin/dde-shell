// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.dtk 1.0
import org.deepin.ds.notificationcenter

SettingActionButton {
    id: root

    property int duration: 200
    property bool textVisible: root.hovered

    contentItem: Item {
        clip: true
        implicitWidth: root.textVisible ? textDelegate.width : iconDelegate.width
        implicitHeight: iconDelegate.width

        Behavior on implicitWidth {
            NumberAnimation { duration: root.duration; easing.type: root.textVisible ? Easing.OutQuad : Easing.InQuad }
        }

        AnimationDelegate {
            id: textDelegate
            leftPadding: 4
            rightPadding: 4
            anchors {
                verticalCenter: parent.verticalCenter
            }
            itemVisible: root.textVisible
            contentItem: Text {
                text: root.text
                font: DTK.fontManager.t10
                color: palette.windowText
            }
        }

        AnimationDelegate {
            id: iconDelegate
            anchors {
                verticalCenter: parent.verticalCenter
            }
            itemVisible: !root.textVisible
            contentItem: DciIcon {
                name: root.icon.name
                sourceSize: Qt.size(root.icon.width, root.icon.height)
                palette: DTK.makeIconPalette(root.palette)
                mode: root.ColorSelector.controlState
                theme: root.ColorSelector.controlTheme
            }
        }
    }

    component AnimationDelegate: Control {
        id: delegate
        required property bool itemVisible

        states: [
            State {
                name: "visible"
                when: delegate.itemVisible
                PropertyChanges { target: delegate; opacity: 1.0 }
                PropertyChanges { target: delegate; visible: true }
            },
            State {
                name: "invisible"
                when: !delegate.itemVisible
                PropertyChanges { target: delegate; opacity: 0.0 }
            }
        ]
        transitions: [
            Transition {
                to: "invisible"
                SequentialAnimation {
                    NumberAnimation { property: "opacity"; easing.type: Easing.InQuad; duration: root.duration }
                    PropertyAction { target: delegate; property: "visible"; value: false }
                }
            },
            Transition {
                NumberAnimation { property: "opacity"; easing.type: Easing.OutQuad; duration: root.duration }
            }
        ]
    }
}
