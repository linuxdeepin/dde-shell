// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

Control {
    id: root
    property bool itemVisible: true

    width: 16
    height: 16

    x: (index % columnCount) * (16 + 10)
    y: Math.floor(index / columnCount) * (16 + 10)
    Behavior on x {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }
    Behavior on y {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }
    states: [
        State {
            when: root.itemVisible
            PropertyChanges { target: root; opacity: 1.0 }
            PropertyChanges { target: root; visible: true }
        },
        State {
            name: "item-invisible"
            when: !root.itemVisible
            PropertyChanges { target: root; opacity: 0.0 }
        }
    ]
    transitions: [
        Transition {
            to: "item-invisible"
            SequentialAnimation {
                NumberAnimation { property: "opacity"; easing.type: Easing.InQuad; duration: 200 }
                PropertyAction { target: root; property: "visible"; value: false }
            }
        },
        Transition {
            NumberAnimation { property: "opacity"; easing.type: Easing.OutQuad; duration: 200 }
        }
    ]
}
