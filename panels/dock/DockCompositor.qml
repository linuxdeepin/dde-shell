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

    property alias dockPosition: pluginManager.dockPosition
    property alias dockColorTheme: pluginManager.dockColorTheme

    property ListModel trayPluginSurfaces: ListModel {}
    property ListModel quickPluginSurfaces: ListModel {}
    property ListModel fixedPluginSurfaces: ListModel {}

    property var compositor: waylandCompositor

    signal popupCreated(var popup)

    function removeDockPluginSurface(model, object) {
        for (var i = 0; i < model.count; ++i) {
            if (object === model.get(i).shellSurface) {
                model.remove(i)
                break
            }
        }
    }

    WaylandCompositor {
        id: waylandCompositor
        socketName: "dockplugin"

        PluginManager {
            id: pluginManager

            onPluginSurfaceCreated: (dockPluginSurface) => {
                if (dockPluginSurface.pluginType === Dock.Tray) {
                    trayPluginSurfaces.append({shellSurface: dockPluginSurface})
                } else if (trayPluginSurfaces.pluginType === Dock.Quick) {
                    quickPluginSurfaces.append({shellSurface: dockPluginSurface})
                } else if (dockPluginSurface.pluginType === Dock.Fixed) {
                    fixedPluginSurfaces.append({shellSurface: dockPluginSurface})
                }
            }

            onPluginSurfaceDestroyed: (dockPluginSurface) => {
                removeDockPluginSurface(trayPluginSurfaces, dockPluginSurface)
            }

            onPluginPopupCreated: (popup) => {
                dockCompositor.popupCreated(popup)
            }
        }
    }
}
