// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtWayland.Compositor
import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0

Item {
    id: root

    property var surfaceAcceptor: function (surfaceId) {
        return true
    }
    property int toolTipVOffset: 0

    PanelToolTipWindow {
        id: toolTipWindow
        onVisibleChanged: function (arg) {
            if (arg && menuWindow.visible)
                menuWindow.close()
        }
    }
    PanelToolTip {
        id: toolTip
        toolTipWindow: toolTipWindow

        property alias shellSurface: surfaceLayer.shellSurface
        ShellSurfaceItemProxy {
            id: surfaceLayer
            anchors.centerIn: parent
            autoClose: true
            onSurfaceDestroyed: function () {
                toolTip.close()
            }
        }
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: toolTip.toolTipWindow
        sizeFollowsWindow: false
    }

    PanelMenuWindow {
        id: menuWindow
        onVisibleChanged: function (arg) {
            if (arg && toolTipWindow.visible)
                toolTipWindow.close()
        }
        onXChanged: {
            if (!menu.shellSurface)
                return
            menu.shellSurface.updatePluginGeometry(Qt.rect(x, y, 0, 0))
        }
        onYChanged: {
            if (!menu.shellSurface)
                return
            menu.shellSurface.updatePluginGeometry(Qt.rect(x, y, 0, 0))
        }
    }
    PanelMenu {
        id: menu
        width: menuSurfaceLayer.width
        height: menuSurfaceLayer.height
        menuWindow: menuWindow
        menuX: DockPositioner.x
        menuY: DockPositioner.y

        property alias shellSurface: menuSurfaceLayer.shellSurface
        ShellSurfaceItemProxy {
            id: menuSurfaceLayer
            anchors.centerIn: parent
            autoClose: true
            onSurfaceDestroyed: function () {
                menu.close()
            }
        }
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: menu.menuWindow
        sizeFollowsWindow: false
    }

    Connections {
        target: DockCompositor
        function onPopupCreated(popupSurface)
        {
            let surfaceId = `${popupSurface.pluginId}::${popupSurface.itemKey}`
            if (surfaceAcceptor && !surfaceAcceptor(surfaceId))
                return

            if (popupSurface.popupType === Dock.TrayPopupTypeTooltip) {
                console.log(root.objectName, ": tooltip created", popupSurface.popupType, popupSurface.pluginId)

                toolTip.shellSurface = popupSurface
                toolTip.toolTipX = Qt.binding(function () {
                    return toolTip.shellSurface.x - toolTip.width / 2
                })
                toolTip.toolTipY = Qt.binding(function () {
                    return toolTip.shellSurface.y - toolTip.height - toolTipVOffset
                })
                toolTip.open()
            } else if (popupSurface.popupType === Dock.TrayPopupTypeMenu) {
                console.log(root.objectName, ": menu created", popupSurface.popupType, popupSurface.pluginId)

                menu.shellSurface = popupSurface
                menu.DockPositioner.bounding = Qt.binding(function () {
                    var point = Qt.point(menu.shellSurface.x, menu.shellSurface.y)
                    return Qt.rect(point.x, point.y, menu.width, menu.height)
                })
                menu.open()
            }
        }
    }
}
