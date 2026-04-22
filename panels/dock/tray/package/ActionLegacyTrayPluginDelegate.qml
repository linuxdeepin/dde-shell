// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
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

AppletItemButton {
    id: root
    property alias inputEventsEnabled: surfaceItem.inputEventsEnabled

    property size visualSize: isHorizontal ? Qt.size(pluginItem.implicitWidth, Math.min(itemHeight, pluginItem.implicitHeight))
                                           : Qt.size(Math.min(itemWidth, pluginItem.implicitWidth), pluginItem.implicitHeight)

    readonly property int itemWidth: isHorizontal ? 0 : DDT.TrayItemPositionManager.dockHeight
    readonly property int itemHeight: isHorizontal ? DDT.TrayItemPositionManager.dockHeight : 0

    required property bool itemVisible
    property bool dragable: true

    padding: 0

    visible: !Drag.active && itemVisible
    hoverEnabled: inputEventsEnabled

    function reportSpotlight(point) {
        root.updateSpotlight(point || Qt.point(width / 2, height / 2))
    }

    function scheduleSpotlightClear() {
        spotlightClearTimer.restart()
    }

    function updatePluginMargins()
    {
        pluginItem.plugin.margins = itemPadding
    }

    contentItem: Item {
        id: pluginItem
        property var plugin: DockCompositor.findSurface(model.surfaceId)
        implicitHeight: plugin ? plugin.height : 0
        implicitWidth: plugin ? plugin.width : 0

        function localItemPoint() {
            let current = pluginItem
            let x = 0
            let y = 0
            while (current.parent) {
                x += current.x
                y += current.y
                current = current.parent
            }

            return Qt.point(x, y)
        }

        property var itemScenePoint: {
            Panel.frontendWindowRect
            pluginItem.localItemPoint()
            return pluginItem.mapToItem(null, 0, 0)
        }

        property var itemGlobalPos: {
            if (!pluginItem.Window.window || !surfaceItem.visible) {
                return Qt.point(0, 0)
            }

            Panel.frontendWindowRect
            pluginItem.localItemPoint()
            return pluginItem.mapToGlobal(0, 0)
        }

        HoverHandler {
            id: hoverHandler
            parent: surfaceItem.shellSurfaceItem
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus

            onPointChanged: {
                if (hovered) {
                    spotlightClearTimer.stop()
                    root.reportSpotlight(hoverHandler.point.position)
                }
            }

            onHoveredChanged: {
                if (hovered) {
                    spotlightClearTimer.stop()
                    root.reportSpotlight()
                    return
                }

                if (!root.hovered) {
                    root.scheduleSpotlightClear()
                }
            }
        }
        TapHandler {
            id: tapHandler
            parent: surfaceItem.shellSurfaceItem
        }

        DDT.ShellSurfaceItemProxy {
            id: surfaceItem
            anchors.fill: parent
            shellSurface: pluginItem.plugin

            onWidthChanged: updatePluginItemGeometryTimer.start()
            onHeightChanged: updatePluginItemGeometryTimer.start()
        }

        Component.onCompleted: {
            if (!pluginItem.plugin || !itemVisible)
                return
            updatePluginMargins()
            pluginItem.plugin.updatePluginGeometry(Qt.rect(Math.round(pluginItem.itemScenePoint.x),
                                                           Math.round(pluginItem.itemScenePoint.y),
                                                           Math.round(surfaceItem.width),
                                                           Math.round(surfaceItem.height)))
            pluginItem.plugin.setGlobalPos(Qt.point(Math.round(pluginItem.itemGlobalPos.x),
                                                    Math.round(pluginItem.itemGlobalPos.y)))
        }

        Timer {
            id: updatePluginItemGeometryTimer
            interval: 200
            running: false
            repeat: false
            onTriggered: {
                if (!pluginItem.plugin || !itemVisible)
                    return
                updatePluginMargins()
                if (pluginItem.itemScenePoint.x >= 0 && pluginItem.itemScenePoint.y >= 0) {
                    pluginItem.plugin.updatePluginGeometry(Qt.rect(Math.round(pluginItem.itemScenePoint.x),
                                                                   Math.round(pluginItem.itemScenePoint.y),
                                                                   Math.round(surfaceItem.width),
                                                                   Math.round(surfaceItem.height)))
                }
            }
        }

        Timer {
            id: updatePluginItemPosTimer
            interval: 200
            running: false
            repeat: false
            onTriggered: {
                if (!pluginItem.plugin || !itemVisible)
                    return
                pluginItem.plugin.setGlobalPos(Qt.point(Math.round(pluginItem.itemGlobalPos.x),
                                                        Math.round(pluginItem.itemGlobalPos.y)))
            }
        }

        onItemScenePointChanged: {
            updatePluginItemGeometryTimer.start()
        }

        onItemGlobalPosChanged: {
            updatePluginItemPosTimer.start()
            surfaceItem.fixPosition()
        }

        onVisibleChanged: {
            if (!pluginItem.plugin || !itemVisible)
                return
            updatePluginMargins()
            pluginItem.plugin.setGlobalPos(Qt.point(Math.round(pluginItem.itemGlobalPos.x),
                                                    Math.round(pluginItem.itemGlobalPos.y)))
        }
    }

    D.ColorSelector.hovered: root.inputEventsEnabled && (pluginItem.plugin && pluginItem.plugin.isItemActive || hoverHandler.hovered || root.hovered)
    D.ColorSelector.pressed: tapHandler.pressed

    property Component overlayWindow: QuickDragWindow {
        height: root.visualSize.height
        width: root.visualSize.width

        Loader {
            height: parent.height
            width: parent.width
            active: root.DQuickDrag.isDragging
            sourceComponent: ShellSurfaceItem {
                anchors.centerIn: parent
                shellSurface: pluginItem.plugin
            }
        }
    }

    Drag.dragType: Drag.Automatic
    DQuickDrag.overlay: overlayWindow
    DQuickDrag.active: Drag.active && Qt.platform.pluginName === "xcb"
    DQuickDrag.hotSpotScale: Qt.size(0.5, 1)
    Drag.mimeData: {
        "text/x-dde-shell-tray-dnd-surfaceId": model.surfaceId,
        "text/x-dde-shell-tray-dnd-sectionType": model.sectionType
    }
    Drag.supportedActions: Qt.MoveAction
    Drag.onActiveChanged: {
        // only drag application-tray plugin can activate application-tray action
        if (model.surfaceId.startsWith("application-tray")) {
            DDT.TraySortOrderModel.actionsAlwaysVisible = Drag.active
        }

        if (Qt.platform.pluginName !== "xcb") {
            root.grabToImage(function(result) {
                root.Drag.imageSource = result.url;
            })
        }

        if (!Drag.active) {
            Panel.contextDragging = false
            // reset position on drop
            Qt.callLater(() => { x = 0; y = 0; });
            return
        }
        Panel.contextDragging = true
    }

    onWidthChanged: {
        if (Qt.platform.pluginName !== "xcb") {
            root.grabToImage(function(result) {
                root.Drag.imageSource = result.url;
            })
        }
    }

    onHoveredChanged: {
        if (hovered) {
            spotlightClearTimer.stop()
            reportSpotlight(Qt.point(width / 2, height / 2))
            return
        }

        if (!hoverHandler.hovered) {
            scheduleSpotlightClear()
        }
    }

    Timer {
        id: spotlightClearTimer
        interval: 70
        repeat: false
        onTriggered: {
            if (!root.hovered && !hoverHandler.hovered && !surfaceItem.hovered) {
                root.clearSpotlight()
            }
        }
    }

    DragHandler {
        id: dragHandler
        enabled: dragable
        // To avoid being continuously active in a short period of time
        onActiveChanged: {
            Qt.callLater(function() { root.Drag.active = dragHandler.active })
        }
    }
}
