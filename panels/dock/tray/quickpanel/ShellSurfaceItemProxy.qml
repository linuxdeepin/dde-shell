// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtWayland.Compositor

Item {
    id: root

    property alias shellSurface: surface.shellSurface

    implicitWidth: surface.width
    implicitHeight: surface.height
    ShellSurfaceItem {
        id: surface
        anchors.fill: parent
        onWidthChanged: updateSurfaceSize()
        onHeightChanged: updateSurfaceSize()

        function updateSurfaceSize()
        {
            if (!shellSurface || !(shellSurface.updatePluginGeometry))
                return
            var pos = root.mapToItem(null, 0, 0)
            shellSurface.updatePluginGeometry(Qt.rect(pos.x , pos.y, root.width, root.height))
        }
    }
}

