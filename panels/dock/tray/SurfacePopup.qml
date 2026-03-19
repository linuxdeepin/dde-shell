// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
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
    signal popupCreated(var surfaceId)
    property var pendingTrayMenu: null
    Timer {
        id: trayMenuOpenTimer
        interval: 150
        repeat: false
        onTriggered: {
            if (pendingTrayMenu) {
                pendingTrayMenu.open()
                pendingTrayMenu = null
            }
        }
    }
    Timer {
        id: restoreAutoCloseTimer
        interval: 200
        repeat: false
        onTriggered: {
            if (pendingTrayMenu) {
                pendingTrayMenu.autoCloseOnDeactivate = true
            }
        }
    }
    Connections {
        target: menu
        function onMenuVisibleChanged() {
            if (!menu.menuVisible) {
                MenuHelper.hasTrayMenuOpen = false
                let notifyPanel = DS.applet("org.deepin.ds.notificationcenter")
                if (notifyPanel) {
                    notifyPanel.hasTrayMenuOpen = false
                }
            }
        }
    }
    Connections {
        target: menu.menuWindow
        function onActiveChanged() {
            if (menu.menuWindow && !menu.menuWindow.active && !menu.menuVisible) {
                MenuHelper.hasTrayMenuOpen = false
                let notifyPanel = DS.applet("org.deepin.ds.notificationcenter")
                if (notifyPanel) {
                    notifyPanel.hasTrayMenuOpen = false
                }
            }
        }
    }

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
        onUpdateGeometryFinished: function () {
            if (!menu.shellSurface)
                return
            menu.shellSurface.updatePluginGeometry(Qt.rect(menu.menuWindow.x, menu.menuWindow.y, 0, 0))
        }
    }
    PanelMenu {
        id: menu
        width: menuSurfaceLayer.width
        height: menuSurfaceLayer.height
        menuWindow: menuWindow

        property alias shellSurface: menuSurfaceLayer.shellSurface
        ShellSurfaceItemProxy {
            id: menuSurfaceLayer
            anchors.centerIn: parent
            autoClose: true
            onSurfaceDestroyed: function () {
                menu.close()
            }
        }

        onMenuVisibleChanged: {
            if (menuVisible) {
                subMenuLoaderDelayTimer.stop()
                subMenuLoader.active = true
            } else {
                subMenuLoaderDelayTimer.start()
            }
        }

        Loader {
            id: subMenuLoader
            active: false
            sourceComponent: SurfaceSubPopup {
                objectName: "stashed's subPopup"
                transientParent: menuWindow
                surfaceAcceptor: function (surfaceId) {
                    if (root.surfaceAcceptor && !root.surfaceAcceptor(surfaceId))
                        return false
                    return true
                }
            }

            // Avoid protocol errors caused by d
            Timer {
                id: subMenuLoaderDelayTimer
                interval: 1000
                repeat: false
                running: false
                onTriggered: function () {
                    subMenuLoader.active = false
                }
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
        function onPopupCreated(popupSurface) {
            let surfaceId = `${popupSurface.pluginId}::${popupSurface.itemKey}`
            if (surfaceAcceptor && !surfaceAcceptor(surfaceId))
                return

            popupCreated(surfaceId)

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
                menu.menuX = Qt.binding(function () {
                    return menu.shellSurface.x
                })
                menu.menuY = Qt.binding(function () {
                    return menu.shellSurface.y
                })
                let notifyPanel = DS.applet("org.deepin.ds.notificationcenter")
                if (notifyPanel && notifyPanel.visible) {
                    notifyPanel.close()
                }
                menu.autoCloseOnDeactivate = false
                MenuHelper.hasTrayMenuOpen = true
                if (notifyPanel) {
                    notifyPanel.hasTrayMenuOpen = true
                }
                root.pendingTrayMenu = menu
                trayMenuOpenTimer.restart()
                restoreAutoCloseTimer.restart()
            }
        }
    }
}
