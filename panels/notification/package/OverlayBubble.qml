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

    contentItem: ColumnLayout {
        spacing: 0
        NormalBubble {
            id: bubbleContent
            bubble: control.bubble
        }
        Repeater {
            model: bubble.overlayCount
            Rectangle {
                Layout.topMargin: -30
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: bubbleContent.width - (index + 1) * 40
                Layout.preferredHeight: 50
                radius: 18
                opacity: 0.8
                z: 1 - bubble.level - (index + 1)
            }
        }
    }

    z: bubble.level <= 1 ? 0 : 1 - bubble.level

    background: Rectangle {
        implicitWidth: 600
        radius: 18
        opacity: 1
        color: "transparent"
    }
}
