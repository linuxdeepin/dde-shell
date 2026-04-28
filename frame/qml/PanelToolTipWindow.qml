// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D

PanelPopupWindow {
    id: root

    flags: Qt.ToolTip | Qt.WindowStaysOnTopHint
    D.DWindow.windowRadius: 8
    D.DWindow.shadowRadius: 8
    D.DWindow.shadowOffset: Qt.point(0, 8)

    function resetVisualState() {
        root.opacity = 1.0
        if (root.contentItem) {
            root.contentItem.opacity = 1.0
            root.contentItem.scale = 1.0
        }
    }

    function showAnimated() {
        resetVisualState()
        root.positionXOffset = root.xOffset
        root.positionYOffset = root.yOffset
        root.show()
    }

    function closeAnimated() {
        if (!root.visible) {
            root.currentItem = null
            return
        }

        root.close()
        root.currentItem = null
    }

    onXOffsetChanged: root.positionXOffset = root.xOffset
    onYOffsetChanged: root.positionYOffset = root.yOffset

    onCurrentItemChanged: {
        if (!!root.currentItem && root.visible) {
            root.positionXOffset = root.xOffset
            root.positionYOffset = root.yOffset
            return
        }

        root.positionXOffset = root.xOffset
        root.positionYOffset = root.yOffset
    }

    onVisibleChanged: {
        if (!visible) {
            resetVisualState()
            return
        }

        root.positionXOffset = root.xOffset
        root.positionYOffset = root.yOffset
        resetVisualState()
    }
}
