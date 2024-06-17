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

    property var itemGlobalPoint: {
        var a = root
        var x = 0, y = 0
        while(a.parent) {
            x += a.x
            y += a.y
            a = a.parent
        }

        return Qt.point(x, y)
    }

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
        if (stashedPopup.popupVisible) {
            stashedPopup.close()
        } else {
            stashedPopup.popupX = itemGlobalPoint.x + root.width / 2
            stashedPopup.open()
        }
        
    }
}
