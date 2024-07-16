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
            if (arg && popupWindow.visible)
                popupWindow.close()
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

    PanelPopupWindow {
        id: popupWindow
        onVisibleChanged: function (arg) {
            if (arg && toolTipWindow.visible)
                toolTipWindow.close()
        }
        onXChanged: {
            if (!popup.shellSurface)
                return
            popup.shellSurface.updatePluginGeometry(Qt.rect(x, y, 0, 0))
        }
        onYChanged: {
            if (!popup.shellSurface)
                return
            popup.shellSurface.updatePluginGeometry(Qt.rect(x, y, 0, 0))
        }
    }
    PanelPopup {
        id: popup
        width: popupSurfaceLayer.width
        height: popupSurfaceLayer.height
        popupWindow: popupWindow

        property alias shellSurface: popupSurfaceLayer.shellSurface
        ShellSurfaceItemProxy {
            id: popupSurfaceLayer
            anchors.centerIn: parent
            autoClose: true
            onSurfaceDestroyed: function () {
                popup.close()
            }
        }
    }

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: popup.popupWindow
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

                popup.shellSurface = popupSurface
                popup.popupX = Qt.binding(function () {
                    return popup.shellSurface.x
                })
                popup.popupY = Qt.binding(function () {
                    return popup.shellSurface.y
                })
                popup.open()
            }
        }
    }
}
