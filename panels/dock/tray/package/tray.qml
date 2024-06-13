// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWayland.Compositor
import Qt.labs.platform 1.1 as LP
import org.deepin.dtk 1.0 as D

import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.ds.dock.tray 1.0 as DDT
import org.deepin.ds.dock.tray.quickpanel 1.0 as TQP

AppletItem {
    id: tray

    property bool useColumnLayout: Panel.position % 2
    property int dockOrder: 25
    implicitWidth: useColumnLayout ? Panel.rootObject.dockSize : overflowId.implicitWidth
    implicitHeight: useColumnLayout ? overflowId.implicitHeight : Panel.rootObject.dockSize

    PanelPopup {
        id: popup
        width: popupContent.width
        height: popupContent.height

        function delayOpen(interval) {
            if (interval > 0) {
                delayTimer.interval = interval
                delayTimer.start()
            } else {
                if (delayTimer.running) {
                    delayTimer.stop()
                }
                open()
            }
        }

        Item {
            anchors.fill: parent
            ShellSurfaceItem {
                id: popupContent
                anchors.centerIn: parent
                onSurfaceDestroyed: {
                    popup.close()
                }
            }

            Timer {
                id: delayTimer
                onTriggered: {
                    popup.open()
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

        TrayContainer {
            isHorizontal: !tray.useColumnLayout
            model: DDT.TraySortOrderModel
            collapsed: DDT.TraySortOrderModel.collapsed
            color: "transparent"
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
            if (popupSurface.popupType === Dock.TrayPopupTypeTooltip) {
                popup.delayOpen(100)
            } else {
                popup.delayOpen(0)
            }
        }
    }

    Connections {
        target: DockCompositor
        function onPluginSurfacesUpdated() {
            let surfacesData = []
            for (let i = 0; i < DockCompositor.trayPluginSurfaces.count; i++) {
                let item = DockCompositor.trayPluginSurfaces.get(i).shellSurface
                let surfaceId = `${item.pluginId}::${item.itemKey}`
                surfacesData.push({"surfaceId": surfaceId, "delegateType": "legacy-tray-plugin"})
                console.log(surfaceId, item, item.pluginId, "surfaceId")
            }
            DDT.TraySortOrderModel.availableSurfaces = surfacesData
            console.log("onPluginSurfacesUpdated", surfacesData)
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