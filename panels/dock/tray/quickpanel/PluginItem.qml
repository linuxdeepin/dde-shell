// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

import org.deepin.dtk 1.0

Control {
    id: root
    property alias shellSurface: surfaceLayer.shellSurface

    DragItem {
        id: dragLayer
        anchors.fill: parent
        dragItem: root
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
