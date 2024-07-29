// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.dtk 1.0 as D

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0

AppletItemButton {
    id: root

    icon.name: {
        switch (Panel.position) {
            case Dock.Right: return "arrow-left"
            case Dock.Left: return "arrow-right"
            case Dock.Top: return "arrow-down"
            case Dock.Bottom: return "arrow-up"
        }
    }

    padding: itemPadding

    property var itemGlobalPoint: {
        var a = root
        var x = 0, y = 0
        while(a.parent) {
            x += a.x
            y += a.y
            a = a.parent
        }

        return Qt.point(x + width / 2, y + height / 2)
    }

    onItemGlobalPointChanged: {
        stashedPopup.collapsedBtnCenterPoint = itemGlobalPoint
    }

    states: [
        State {
            name: "opened"
            PropertyChanges { target: root.contentItem; rotation: 180 }
        },

        State {
            name: "closed"
            PropertyChanges { target: root.contentItem; rotation: 0 }
        }
    ]

    transitions: [
        Transition {
            RotationAnimation { duration: 200; }
        }
    ]

    Binding {
        target: root
        property: "state"
        value: stashedPopup.popupVisible ? "opened" : "closed"
        when: stashedPopup.popupVisibleChanged
    }

    onClicked: {
        stashedPopup.open()
    }
}
