// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
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
    property alias transientParent: menuWindow.transientParent

    WaylandOutput {
        compositor: DockCompositor.compositor
        sizeFollowsWindow: false
        window: PanelMenuWindow {
            id: menuWindow
            objectName: "subMenu"
            mainMenuWindow: Panel.menuWindow
            updateGeometryer : function () {
                if (menuWindow.requestedWidth <= 10 || menuWindow.requestedHeight <= 10) {
                    menuWindow.width = menuWindow.requestedWidth
                    menuWindow.height = menuWindow.requestedHeight
                    return
                }

                // following transientParent's screen.
                menuWindow.screen = menuWindow.transientParent.screen

                let bounding = Qt.rect(menuWindow.screen.virtualX + margins, menuWindow.screen.virtualY + margins,
                                       menuWindow.screen.width - margins * 2, menuWindow.screen.height - margins * 2)
                let pos = Qt.point(xOffset, yOffset)
                let newX = selectValue(pos.x, bounding.left, bounding.right - menuWindow.requestedWidth)
                let newY = selectValue(pos.y, bounding.top, bounding.bottom - menuWindow.requestedHeight)
                menuWindow.setWindowGeometry(newX, newY, menuWindow.requestedWidth, menuWindow.requestedHeight)
            }
            onUpdateGeometryFinished: function () {
                if (!popup.shellSurface)
                    return
                popup.shellSurface.updatePluginGeometry(Qt.rect(popup.menuWindow.x, popup.menuWindow.y, 0, 0))
            }

            PanelMenu {
                id: popup
                width: popupSurfaceLayer.width
                height: popupSurfaceLayer.height
                menuWindow: menuWindow

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
        }
    }

    Connections {
        target: DockCompositor
        function onPopupCreated(popupSurface) {
            let surfaceId = `${popupSurface.pluginId}::${popupSurface.itemKey}`
            if (surfaceAcceptor && !surfaceAcceptor(surfaceId))
                return

            if (popupSurface.popupType === Dock.TrayPopupTypeSubPopup) {

                popup.shellSurface = popupSurface
                popup.menuX = Qt.binding(function () {
                    return popup.shellSurface.x
                })
                popup.menuY = Qt.binding(function () {
                    return popup.shellSurface.y
                })
                popup.open()
            }
        }
    }
}
