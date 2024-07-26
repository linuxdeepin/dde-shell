// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.dtk 1.0 as D

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.ds.dock.tray 1.0 as DDT

D.ToolButton {
    id: root
    property bool isDropHover: model.visualIndex === dropHoverIndex && dropHoverIndex !== -1

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

    D.ColorSelector.hovered: (isDropHover && DDT.TraySortOrderModel.actionsAlwaysVisible) || hoverHandler.hovered

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

    HoverHandler {
        id: hoverHandler
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

    onIsDropHoverChanged: {
        if (isDropHover && !stashedPopup.popupVisible) {
            stashedPopup.dropHover = false
            stashedPopup.open()
        }
    }

    Timer {
        id: closeStashPopupTimer
        running: !isDropHover && !stashedPopup.dropHover && !stashedPopup.stashItemDragging
        interval: 300
        repeat: false
        onTriggered: {
            stashedPopup.close()
        }
    }

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
