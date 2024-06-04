// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWayland.Compositor
import Qt.labs.platform 1.1 as LP

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0
import org.deepin.ds.dock.tray.quickpanel 1.0 as TQP

AppletItem {
    id: tray

    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: 25
    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : trayContainter.suggestedImplicitWidth
    implicitHeight: useColumnLayout ? trayContainter.suggestedImplicitHeight : Panel.rootObject.dockSize

    PanelPopup {
        id: popup
        width: popupContent.width
        height: popupContent.height

        Item {
            anchors.fill: parent
            ShellSurfaceItem {
                id: popupContent
                anchors.centerIn: parent
                onSurfaceDestroyed: {
                    popup.close()
                }
            }
        }
    }

    GridLayout {
        id: overflowId
        columns: 1
        rows: 1
        anchors.centerIn: parent
        flow: useColumnLayout ? GridLayout.LeftToRight : GridLayout.TopToBottom
        property bool trayVisible: true
        columnSpacing: 10
        rowSpacing: 10
        OverflowContainer {
            id: trayContainter
            useColumnLayout: tray.useColumnLayout
            model: DockCompositor.trayPluginSurfaces
            spacing: 10
            delegate: Item {
                id: pluginItem
                property var plugin: modelData
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
                    shellSurface: plugin
                }

                Component.onCompleted: {
                    plugin.updatePluginGeometry(Qt.rect(itemGlobalPoint.x, itemGlobalPoint.y, surfaceItem.width, surfaceItem.height))
                }

                Timer {
                    id: updatePluginItemGeometryTimer
                    interval: 500
                    running: false
                    repeat: false
                    onTriggered: {
                        if (itemGlobalPoint.x > 0 && itemGlobalPoint.y > 0) {
                            plugin.updatePluginGeometry(Qt.rect(itemGlobalPoint.x, itemGlobalPoint.y, surfaceItem.width, surfaceItem.height))
                        }
                    }
                }

                onItemGlobalPointChanged: {
                    updatePluginItemGeometryTimer.start()
                }
            }
        }

        TQP.QuickPanel { }
    }

    Connections {
        target: DockCompositor
        function onPopupCreated(popupSurface)
        {
            // Embed popup is plugin's child page in quickpanel
            if (popupSurface.popupType === Dock.TrayPopupTypeEmbed)
                return

            popupContent.shellSurface = popupSurface
            popup.popupX = popupSurface.x
            popup.popupY = popupSurface.popupType === Dock.TrayPopupTypeMenu ? popupSurface.y : 0
            popup.open()
        }
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: Panel.rootObject
        sizeFollowsWindow: true
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: Panel.popupWindow
        sizeFollowsWindow: false
    }
}