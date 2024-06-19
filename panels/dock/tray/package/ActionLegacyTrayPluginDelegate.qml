// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWayland.Compositor
import Qt.labs.platform 1.1 as LP
import org.deepin.dtk 1.0 as D

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.ds.dock.tray 1.0 as DDT

Button {
    property alias inputEventsEnabled: surfaceItem.inputEventsEnabled

    x: isHorizontal ? (model.visualIndex * (16 + 10)) : 0
    y: !isHorizontal ? (model.visualIndex * (16 + 10)) : 0
    icon.width: 16
    icon.height: 16
    width: 16
    height: 16

    contentItem: Item {
        id: pluginItem
        property var plugin: DockCompositor.findSurface(model.surfaceId)
        property var popupShow: false
        implicitHeight: surfaceItem.height
        implicitWidth: surfaceItem.width

        property var itemGlobalPoint: {
            var a = pluginItem
            var x = 0, y = 0
            while(a.parent) {
                x += a.x
                y += a.y
                a = a.parent
            }

            return Qt.point(x, y)
        }

        ShellSurfaceItem {
            id: surfaceItem
            anchors.centerIn: parent
            shellSurface: pluginItem.plugin
        }

        Component.onCompleted: {
            pluginItem.plugin.updatePluginGeometry(Qt.rect(pluginItem.itemGlobalPoint.x, pluginItem.itemGlobalPoint.y, 16, 16))
        }

        Timer {
            id: updatePluginItemGeometryTimer
            interval: 500
            running: false
            repeat: false
            onTriggered: {
                if (pluginItem.itemGlobalPoint.x > 0 && pluginItem.itemGlobalPoint.y > 0) {
                    pluginItem.plugin.updatePluginGeometry(Qt.rect(pluginItem.itemGlobalPoint.x, pluginItem.itemGlobalPoint.y, 16, 16))
                }
            }
        }

        onItemGlobalPointChanged: {
            updatePluginItemGeometryTimer.start()
        }
    }

    Drag.active: dragHandler.active
    Drag.dragType: Drag.Automatic
    Drag.mimeData: {
        "text/x-dde-shell-tray-dnd-surfaceId": model.surfaceId
    }
    Drag.supportedActions: Qt.MoveAction
    Drag.onActiveChanged: {
        DDT.TraySortOrderModel.actionsAlwaysVisible = Drag.active
        if (!Drag.active) {
            // reset position on drop
            Qt.callLater(() => { x = 0; y = 0; });
        }
    }
    DragHandler {
        id: dragHandler
    }
}
