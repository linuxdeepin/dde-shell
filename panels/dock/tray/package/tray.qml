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

    PanelToolTip {
        id: toolTip
        property alias shellSurface: toolTipContent.shellSurface

        ShellSurfaceItem {
            id: toolTipContent
            anchors.centerIn: parent
            onSurfaceDestroyed: {
                toolTip.close()
            }
        }
    }

    PanelPopup {
        id: stashedPopup
        width: stashedContainer.width
        height: stashedContainer.height

        Control {
            id: stashedContainer
            padding: 10
            contentItem: StashContainer {
                color: "transparent"
                model: DDT.SortFilterProxyModel {
                    sourceModel: DDT.TraySortOrderModel
                    filterRowCallback: (sourceRow, sourceParent) => {
                        console.log(sourceRow, sourceParent)
                        let index = sourceModel.index(sourceRow, 0, sourceParent)
                        return sourceModel.data(index, DDT.TraySortOrderModel.SectionTypeRole) === "stashed"
                    }
                }
                anchors.centerIn: parent
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
            if (popupSurface.popupType === Dock.TrayPopupTypeTooltip) {
                toolTip.shellSurface = popupSurface
                toolTip.toolTipX = popupSurface.x
                toolTip.open()
            } else if (popupSurface.popupType === Dock.TrayPopupTypeMenu) {
                popupContent.shellSurface = popupSurface
                popup.popupX = popupSurface.x
                popup.popupY = popupSurface.y
                popup.open()
            } else if (popupSurface.popupType === Dock.TrayPopupTypePanel) {
                popupContent.shellSurface = popupSurface
                popup.popupX = popupSurface.x
                popup.open()
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

    WaylandOutput {
        compositor: DockCompositor.compositor
        window: Panel.toolTipWindow
        sizeFollowsWindow: false
    }
}
