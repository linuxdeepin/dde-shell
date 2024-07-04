// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

import org.deepin.dtk 1.0

Control {
    id: root
    property string pluginId
    property string itemKey
    property alias shellSurface: surfaceLayer.shellSurface
    property alias traySurface: dragLayer.fallbackDragImage
    property bool canDrag: true
    function updateSurface()
    {
        surfaceLayer.updateSurfacePosition()
    }

    // Control's hovered is false when hover ShellSurfaceItem.
    ColorSelector.hovered: hoverHandler.hovered
    DragItem {
        id: dragLayer
        anchors.fill: parent
        dragItem: root
        dragTextData: `${root.pluginId}::${root.itemKey}`
        fallbackIconSize: traySurface ? traySurface.size : Qt.size(16, 16)
        enabledDrag: canDrag
    }

    Connections {
        target: shellSurface
        enabled: shellSurface
        function onRecvMouseEvent(type) {
            // type (2: Qt.MouseButtonPress), (3: Qt.MouseButtonRelease)
            if (type === 2) {
                canDrag = false
            } else if (type === 3) {
                canDrag = true
            }
        }
    }

    HoverHandler {
        id: hoverHandler
        parent: surfaceLayer
    }

    ShellSurfaceItemProxy {
        id: surfaceLayer
        anchors.centerIn: parent
        anchors.fill: parent
        onWidthChanged: updateSurfaceSize()
        onHeightChanged: updateSurfaceSize()

        function updateSurfaceSize()
        {
            if (!shellSurface || !(shellSurface.updatePluginGeometry))
                return
            shellSurface.updatePluginGeometry(Qt.rect(0, 0, surfaceLayer.width, surfaceLayer.height))
        }
        function updateSurfacePosition()
        {
            if (!shellSurface || !(shellSurface.updatePluginGeometry))
                return

            var pos = surfaceLayer.mapToItem(null, 0, 0)
            shellSurface.updatePluginGeometry(Qt.rect(pos.x, pos.y, surfaceLayer.width, surfaceLayer.height))
        }
    }

    background: BoxPanel {
        insideBorderColor: null
        outsideBorderColor: null
        color2: color1
        radius: 10
    }
}
