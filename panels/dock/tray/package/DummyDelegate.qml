// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import Qt.labs.platform 1.1 as LP

Button {
    id: dummyBtn
    property bool itemVisible: {
        if (model.sectionType === "collapsable") return !root.collapsed
        return model.sectionType !== "stashed"
    }
    x: isHorizontal ? (model.visualIndex * (16 + 10)) : 0
    y: !isHorizontal ? (model.visualIndex * (16 + 10)) : 0
    icon.name: model.surfaceId
    icon.width: 16
    icon.height: 16
    width: 16
    height: 16
    Behavior on x {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }
    Behavior on y {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }
    states: [
        State {
            when: dummyBtn.itemVisible
            PropertyChanges { target: dummyBtn; opacity: 1.0 }
            PropertyChanges { target: dummyBtn; visible: true }
        },
        State {
            name: "item-invisible"
            when: !dummyBtn.itemVisible
            PropertyChanges { target: dummyBtn; opacity: 0.0 }
        }
    ]
    transitions: [
        Transition {
            to: "item-invisible"
            SequentialAnimation {
                NumberAnimation { property: "opacity"; easing.type: Easing.InQuad; duration: 200 }
                PropertyAction { target: dummyBtn; property: "visible"; value: false }
            }
        },
        Transition {
            NumberAnimation { property: "opacity"; easing.type: Easing.OutQuad; duration: 200 }
        }
    ]
}
