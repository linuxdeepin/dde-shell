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

    width: 16
    height: 16
    icon.name: {
        switch (Panel.position) {
            case Dock.Right: return "arrow-left"
            case Dock.Left: return "arrow-right"
            case Dock.Top: return "arrow-down"
            case Dock.Bottom: return "arrow-up"
        }
    }
    icon.width: width
    icon.height: height
    display: D.IconLabel.IconOnly

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
