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
    ColorSelector.family: Palette.CommonColor
    function updateSurface()
    {
        surfaceLayer.updateSurfacePosition()
    }

    DragItem {
        id: dragLayer
        anchors.fill: parent
        dragItem: root
        dragTextData: `${root.pluginId}::${root.itemKey}`
        fallbackIconSize: traySurface ? traySurface.size : Qt.size(16, 16)
    }

    ShellSurfaceItemProxy {
        id: surfaceLayer
        anchors.centerIn: parent
        anchors.fill: parent
    }

    // TODO Control's hovered is false when hover ShellSurfaceItem.
    background: BoxPanel {
        color2: color1
        radius: 10
    }
}
