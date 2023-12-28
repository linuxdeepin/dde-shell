// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma Singleton

import QtQuick
import QtQuick.Controls
import QtWayland.Compositor

import org.deepin.ds 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.ds.dock 1.0

Item {
    id: dockCompositor

    property var tooltipMap: ({})
    property var popupMap: ({})

    property ListModel trayPluginSurfaces: ListModel {}
    property ListModel fixedPluginSurfaces: ListModel {}
    property ListModel systemPluginSurfaces: ListModel {}
    property ListModel toolPluginSurfaces: ListModel {}
    property ListModel quickPluginSurfaces: ListModel {}
    property ListModel slidingPanelPluginSurfaces: ListModel {}

    property var compositor: waylandCompositor

    function handlePluginTooltipSurfaceAdded(shellSurface) {
        tooltipMap[shellSurface.pluginId] = (shellSurface)
    }

    function handleTrayPopupSurfaceAdded(shellSurface) {
        popupMap[shellSurface.pluginId] = (shellSurface)
    }

    function handleDockTrayIconSurfaceAdded(shellSurface) {
        trayPluginSurfaces.append({shellSurface: shellSurface})
    }

    function handleDockFixedPluginSurfaceAdded(shellSurface) {
        fixedPluginSurfaces.append({shellSurface: shellSurface})
    }

    function handleDockSystemPluginSurfacesAdded(shellSurface) {
        systemPluginSurfaces.append({shellSurface: shellSurface})
    }

    function handleDockToolPluginSurfaceAdded(shellSurface) {
        toolPluginSurfaces.append({shellSurface: shellSurface})
    }

    function handleDockQuickPluginSurfaceAdded(shellSurface) {
        quickPluginSurfaces.append({shellSurface: shellSurface})
    }

    function handleDockSlidingPanelPluginSurfaceAdded(shellSurface) {
        slidingPanelPluginSurfaces.append({shellSurface: shellSurface})
    }

    WaylandCompositor {
        id: waylandCompositor
        socketName: "dockplugin"

        DockPluginManager {
            onPluginSurfaceCreated: (dockPluginSurface) => {
                switch(dockPluginSurface.surfaceType) {
                case DockPluginManager.Tooltip:
                    dockCompositor.handlePluginTooltipSurfaceAdded(dockPluginSurface)
                    break;
                case DockPluginManager.Popup:
                    dockCompositor.handleTrayPopupSurfaceAdded(dockPluginSurface)
                    break;
                case DockPluginManager.Tray:
                    dockCompositor.handleDockTrayIconSurfaceAdded(dockPluginSurface)
                    break;
                case DockPluginManager.Fixed:
                    dockCompositor.handleDockFixedPluginSurfaceAdded(dockPluginSurface)
                    break;
                case DockPluginManager.System:
                    dockCompositor.handleDockSystemPluginSurfacesAdded(dockPluginSurface)
                    break;
                case DockPluginManager.Tool:
                    dockCompositor.handleDockToolPluginSurfaceAdded(dockPluginSurface)
                    break;
                case DockPluginManager.Quick:
                    dockCompositor.handleDockQuickPluginSurfaceAdded(dockPluginSurface)
                    break;
                case DockPluginManager.SlidingPanel:
                    dockCompositor.handleDockSlidingPanelPluginSurfaceAdded(dockPluginSurface)
                    break;
                }
            }
        }
    }
}
