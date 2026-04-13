// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk

IconButton {
    id: control
    property bool isActive
    property real radius: 4
    property bool autoClosePopup: false

    padding: 4
    topPadding: undefined
    bottomPadding: undefined
    leftPadding: undefined
    rightPadding: undefined

    textColor: DockPalette.iconTextPalette
    display: IconLabel.IconOnly

    icon.width: 16
    icon.height: 16

    Connections {
        target: control
        enabled: autoClosePopup
        function onClicked() {
            Panel.requestClosePopup()
        }
    }

    background: AppletItemBackground {
        radius: control.radius
        isActive: control.isActive
    }

    Component.onCompleted: {
        contentItem.smooth = false
    }

    property var contentGlobalPoint: {
        var a = contentItem
        if (!a) return Qt.point(0, 0)
        var x = 0, y = 0
        while (a && a.parent) {
            x += a.x
            y += a.y
            a = a.parent
        }

        return Qt.point(x, y)
    }

    PositionFixer {
        id: positionFixer
        item: control
        container: control
    }

    onContentGlobalPointChanged: {
        positionFixer.fix()
    }
}
