// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

D.Control {
    id: control
    property var bubble
    readonly property int radius: 12

    contentItem: ColumnLayout {
        spacing: 0
        Rectangle {
            visible: bubble.level > 2
            Layout.bottomMargin: -34
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: bubbleContent.width - 2 * control.radius
            Layout.preferredHeight: 40
            radius: control.radius
            opacity: 0.8
            z: control.z + control.z + 1
        }

        Rectangle {
            Layout.bottomMargin: -34
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: bubbleContent.width - control.radius
            Layout.preferredHeight: 40
            radius: control.radius
            opacity: 0.8
            z: control.z + control.z + 1
        }

        NormalBubble {
            id: bubbleContent
            Layout.fillWidth: true
            Layout.maximumWidth: 360
            bubble: control.bubble
        }
    }

    z: bubble.level <= 1 ? 0 : 1 - bubble.level

    background: Rectangle {
        implicitWidth: 200
        radius: control.radius
        opacity: 1
        color: "transparent"
    }
}
