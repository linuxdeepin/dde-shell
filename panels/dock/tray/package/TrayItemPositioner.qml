// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.ds.dock.tray 1.0 as DDT

Control {
    id: root
    property bool itemVisible: {
        if (model.sectionType === "collapsable") return !collapsed && model.visibility
        return model.sectionType !== "stashed" && model.visibility
    }
    property size visualSize: Qt.size(0, 0)

    property point visualPosition: DDT.TrayItemPositionRegister.visualPosition
    DDT.TrayItemPositionRegister.visualIndex: model.visualIndex
    DDT.TrayItemPositionRegister.visualSize: Qt.size(width, height)

    width: visualSize.width !== 0 ? visualSize.width : DDT.TrayItemPositionManager.itemVisualSize.width
    height: visualSize.height !== 0 ? visualSize.height : DDT.TrayItemPositionManager.itemVisualSize.height

    x: visualPosition.x
    y: visualPosition.y
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
