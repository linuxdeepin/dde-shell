// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtWayland.Compositor
import Qt.labs.platform 1.1 as LP

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock.compositor 1.0
import org.deepin.ds.dock 1.0

AppletItem {
    id: tray
    visible: true

    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: 25

    implicitWidth: useColumnLayout ? Panel.dockSize : trayContainter.implicitWidth
    implicitHeight: useColumnLayout ? trayContainter.implicitHeight : Panel.dockSize

    PanelPopup {
        id: popup
        width: popupContent.width
        height: popupContent.height

        ShellSurfaceItem {
            id: popupContent
        }
    }

    OverflowContainer {
        id: trayContainter
        anchors.centerIn: parent
        useColumnLayout: tray.useColumnLayout
        model: DockCompositor.trayPluginSurfaces
        delegate: Item {
            id: pluginItem
            property var plugin: modelData
            property var popupShow: false
            implicitHeight: 32
            implicitWidth: 32

            ShellSurfaceItem {
                anchors.centerIn: parent
                width: 28
                height: 28
                shellSurface: plugin
                onSurfaceDestroyed: {
                    delete DockCompositor.tooltipMap[plugin.pluginId]
                    delete DockCompositor.popupMap[plugin.pluginId]
                    DockCompositor.trayPluginSurfaces.remove(index)
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                    onEntered: {
                        var toolTipsurface = DockCompositor.tooltipMap[plugin.pluginId]
                        if (toolTipsurface !== undefined) {
                            popupContent.shellSurface = toolTipsurface
                            var itemPos = tray.getItemPopupPosition(pluginItem)
                            popup.x = itemPos.x
                            popup.y = itemPos.y
                            console.log(itemPos)
                            popup.open()
                        }
                    }

                    onClicked: mouse => {
                        var pluginPopup = DockCompositor.popupMap[plugin.pluginId]
                        if (pluginPopup === undefined) {
                            plugin.click("", 0)
                        } else {
                            popupContent.shellSurface = pluginPopup
                            popupShow = true
                            var itemPos = tray.getItemPopupPosition(pluginItem)
                            popup.x = itemPos.x
                            popup.y = itemPos.y
                            console.log(itemPos)
                            popup.open()
                        }
                    }

                    onExited: {
                        if (!popupShow) {
                            popup.close()
                        }
                    }
                }
            }
        }
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: Panel.rootObject
        sizeFollowsWindow: true
    }

    function getItemPopupPosition(item) {
        var itemPos = item.mapToItem(null,0,0)

        switch (Panel.position) {
        case Dock.Top:
            itemPos.x -= popup.width / 2
            itemPos.y += Panel.dockSize
            break
        case Dock.Right:
            itemPos.x -= (popup.width + 10)
            itemPos.y -= popup.height / 2
            break
        case Dock.Bottom:
            itemPos.x -= popup.width / 2
            itemPos.y -= (popup.height + 10)
            break
        case Dock.Left:
            itemPos.x += Panel.dockSize
            itemPos.y -= popup.height / 2
            break
        }

        return itemPos
    }
}
