// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk.style 1.0 as DStyle

QtObject {
    id: root

    property bool useColumnLayout: false
    property int itemAlignment: Dock.CenterAlignment
    property int position: Dock.Bottom
    property int hideState: Dock.Show
    property real dockSize: 0
    property real dockItemIconSize: 0
    property real devicePixelRatio: 1
    property int screenWidth: 0
    property int colorTheme: Dock.Light
    property bool dragging: false
    property var gridLayout: null
    property var rightPart: null

    readonly property bool enabled: !useColumnLayout
        && itemAlignment === Dock.FashionAlignment
        && (position === Dock.Bottom || position === Dock.Top)
    readonly property bool topMode: enabled && position === Dock.Top
    readonly property bool bottomMode: enabled && position === Dock.Bottom
    readonly property int floatingMargin: 8
    readonly property int verticalPadding: Math.max(6, Math.round(dockSize * 0.16))
    readonly property int surfaceThickness: useColumnLayout ? dockSize : dockSize + verticalPadding * 2 
    readonly property int backgroundRadius: Math.round(surfaceThickness / 4)
    readonly property real gridDisplayedWidth: enabled && gridLayout
        ? gridLayout.implicitWidth
        : 0
    readonly property real gridDisplayedHeight: enabled && gridLayout
        ? gridLayout.implicitHeight
        : 0
    readonly property real contentWidth: {
        if (!enabled) {
            return 0
        }

        let width = gridDisplayedWidth
        if (rightPart && rightPart.visible) {
            if (width > 0) {
                width += gridLayout.columnSpacing
            }
            width += rightPart.implicitWidth
        }
        return width
    }
    readonly property bool widthAnimationEnabled: enabled && !dragging

    function effectiveShellWidth() {
        if (!enabled) {
            return 0
        }

        return Math.min(contentWidth, screenWidth)
    }
}
