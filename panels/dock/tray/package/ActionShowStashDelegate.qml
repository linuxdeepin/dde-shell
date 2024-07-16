// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.dtk 1.0 as D

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0

D.ToolButton {
    id: root

    icon.name: {
        switch (Panel.position) {
            case Dock.Right: return "arrow-left"
            case Dock.Left: return "arrow-right"
            case Dock.Top: return "arrow-down"
            case Dock.Bottom: return "arrow-up"
        }
    }
    icon.width: 16
    icon.height: 16
    display: D.IconLabel.IconOnly

    topPadding: itemPadding
    bottomPadding: itemPadding
    leftPadding: itemPadding
    rightPadding: itemPadding

    states: [
        State {
            name: "opened"
            PropertyChanges { target: root; rotation: 180 }
        },

        State {
            name: "closed"
            PropertyChanges { target: root; rotation: 0 }
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
        var point = root.mapToItem(null, root.width / 2, 0)
        stashedPopup.popupX = Qt.binding(function () {
            return point.x - stashedPopup.width / 2
        })
        stashedPopup.popupY = Qt.binding(function () {
            return -stashedPopup.height - 10
        })
        stashedPopup.open()
    }
}
