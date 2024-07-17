// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWayland.Compositor
import Qt.labs.platform 1.1 as LP
import org.deepin.dtk 1.0 as D
import org.deepin.dtk.private 1.0 as DP

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.ds.dock.tray 1.0 as DDT

Button {
    property alias inputEventsEnabled: surfaceItem.inputEventsEnabled

    property size visualSize: Qt.size(pluginItem.implicitWidth + itemPadding * 2, pluginItem.implicitHeight + itemPadding * 2)

    readonly property int itemWidth: isHorizontal ? 0 : DDT.TrayItemPositionManager.dockHeight
    readonly property int itemHeight: isHorizontal ? DDT.TrayItemPositionManager.dockHeight : 0

    topPadding: itemPadding
    bottomPadding: itemPadding
    leftPadding: itemPadding
    rightPadding: itemPadding

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
        
        property var itemGlobalPos: {
            var a = pluginItem
            var x = 0, y = 0

            if (a.Window.window && surfaceItem.visible) {
                while (a.parent) {
                    x += a.x
                    y += a.y
                    a = a.parent
                }
                x += pluginItem.Window.window.x
                y += pluginItem.Window.window.y

            }

            return Qt.point(x, y)
        }

        HoverHandler {
            id: hoverHandler
            parent: surfaceItem
        }

        ShellSurfaceItem {
            id: surfaceItem
            anchors.centerIn: parent
            shellSurface: pluginItem.plugin
            inputEventsEnabled: !DDT.TraySortOrderModel.collapsed
        }

        Component.onCompleted: {
            if (!pluginItem.plugin)
                return
            pluginItem.plugin.updatePluginGeometry(Qt.rect(pluginItem.itemGlobalPoint.x, pluginItem.itemGlobalPoint.y, 0, 0))
            pluginItem.plugin.setGlobalPos(pluginItem.itemGlobalPos)
        }

        Timer {
            id: updatePluginItemGeometryTimer
            interval: 500
            running: false
            repeat: false
            onTriggered: {
                if (!pluginItem.plugin)
                    return
                if (pluginItem.itemGlobalPoint.x > 0 && pluginItem.itemGlobalPoint.y > 0) {
                    pluginItem.plugin.updatePluginGeometry(Qt.rect(pluginItem.itemGlobalPoint.x, pluginItem.itemGlobalPoint.y, 0, 0))
                }
            }
        }

        Timer {
            id: updatePluginItemPosTimer
            interval: 500
            running: false
            repeat: false
            onTriggered: {
                if (!pluginItem.plugin)
                    return
                pluginItem.plugin.setGlobalPos(pluginItem.itemGlobalPos)
            }
        }

        onItemGlobalPointChanged: {
            updatePluginItemGeometryTimer.start()
        }

        onItemGlobalPosChanged: {
            updatePluginItemPosTimer.start()
        }

        onVisibleChanged: {
            if (!pluginItem.plugin)
                return
            pluginItem.plugin.setGlobalPos(pluginItem.itemGlobalPos)
        }
    }

    D.ColorSelector.hovered: pluginItem.plugin && pluginItem.plugin.isItemActive || hoverHandler.hovered
    background: D.BoxPanel {
        property D.Palette backgroundPalette: DockPalette.backgroundPalette

        color2: color1
        color1: backgroundPalette

        outsideBorderColor: null
        insideBorderColor: null
    }

    property Component overlayWindow: QuickDragWindow {
        height: parent.visualSize.height
        width: parent.visualSize.width
        Item {
            height: parent.height
            width: parent.width
            ShellSurfaceItem {
                anchors.centerIn: parent
                shellSurface: pluginItem.plugin
            }
        }
    }

    Drag.active: dragHandler.active
    Drag.dragType: Drag.Automatic
    DQuickDrag.overlay: overlayWindow
    DQuickDrag.active: Drag.active
    DQuickDrag.hotSpotScale: Qt.size(0.5, 1)
    Drag.mimeData: {
        "text/x-dde-shell-tray-dnd-surfaceId": model.surfaceId
    }
    Drag.supportedActions: Qt.MoveAction
    Drag.onActiveChanged: {
        DDT.TraySortOrderModel.actionsAlwaysVisible = Drag.active
        pluginItem.visible = !Drag.active
        if (!Drag.active) {
            // reset position on drop
            Qt.callLater(() => { x = 0; y = 0; });
        }
    }
    DragHandler {
        id: dragHandler
    }
}
